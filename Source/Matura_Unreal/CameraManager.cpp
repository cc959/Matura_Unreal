// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraManager.h"

#include <filesystem>
#include <fstream>

using namespace std;
#include <thread>
#include <deque>


CameraManager::CameraManager(class ABall* ball) : ball(ball)
{
	Thread = FRunnableThread::Create(this, TEXT("Camera Thread"));
}

CameraManager::~CameraManager()
{
	if (Thread)
	{
		run_thread = false;
		Thread->Kill();
		delete Thread;
	}
}

bool CameraManager::Init()
{
	return FRunnable::Init();
}

void SwapRows(Mat& mat, int a, int b)
{
	Mat temp = mat.row(a) + 0; // always adding zero to convert it into a MatExpr, which then gets copied proplerly
	mat.row(a) = mat.row(b) + 0;
	mat.row(b) = temp + 0;
}

Mat ConvertToCameraMatrix(FTransform transform)
{
	const FVector translation = transform.GetTranslation();
	auto rotation = transform.GetRotation().Euler();
	rotation *= CV_PI / 180.; // convert to radians

	Mat cv_translation = Mat(Vec3d(translation.Y, -translation.Z, translation.X));

	Mat cv_rotation_matrix = Mat::eye(3, 3, CV_64F);
	cv_rotation_matrix = (Mat_<double>(3, 3) << cos(rotation.X), -sin(rotation.X), 0, sin(rotation.X), cos(rotation.X),
		0, 0, 0, 1) * cv_rotation_matrix;
	cv_rotation_matrix = (Mat_<double>(3, 3) << 1, 0, 0, 0, cos(rotation.Y), -sin(rotation.Y), 0, sin(rotation.Y),
		cos(rotation.Y)) * cv_rotation_matrix;
	cv_rotation_matrix = (Mat_<double>(3, 3) << cos(rotation.Z), 0, sin(rotation.Z), 0, 1, 0, -sin(rotation.Z), 0,
		cos(rotation.Z)) * cv_rotation_matrix;

	Mat RT;
	hconcat(cv_rotation_matrix, cv_translation, RT);

	Mat floor = (Mat_<double>(1, 4) << 0, 0, 0, 1);
	vconcat(RT, floor, RT);
	RT = RT.inv();

	return RT.rowRange(0, 3);
}

void CameraManager::CameraLoop(ATrackingCamera* camera, deque<Detection>* ball_2d_detections)
{
	TFuture<pair<FTransform, map<ATag*, FMatrix>>> transform_future;
	int64_t last_now = chrono::high_resolution_clock::now().time_since_epoch().count();


	while (!camera || !camera->loaded)
	{
		if (!run_thread)
			return;
		UE_LOG(LogTemp, Error, TEXT("%s is not ready yet or null"), *camera->camera_path)
		usleep(10000); //wait 10 ms
	}

	camera->destroy_lock.lock();

	camera->must_update_tags = true;
	while (camera->must_update_tags)
		usleep(10000); //wait 10 ms
	
	UE_LOG(LogTemp, Error, TEXT("%s loop has started"), *camera->camera_path);

	camera->in_use = true;
	
	while (run_thread)
	{
		// camera wants to be released, but is waiting for this thread to finish
		if (!camera->loaded)
		{
			if (transform_future.IsValid())
				transform_future.Wait();
			UE_LOG(LogTemp, Error, TEXT("%s is not loaded anymore, exiting thread"), *camera->camera_path);
			camera->destroy_lock.unlock();
			return;
		}
		
		double time = camera->SyncFrame() / 1000.;
		camera->GetFrame();

		Point2d ball_position = camera->FindBall();
		

		ball_detection_2d_mut.lock();
		camera->last_frame_time = time;
		
		while (ball_2d_detections->size() && abs(ball_2d_detections->front().time - time) > 0.1)
			ball_2d_detections->pop_front();

		if (ball_position != Point2d{-1, -1})
			ball_2d_detections->push_back({ball_position, time});
		ball_detection_2d_mut.unlock();

		camera->DrawDetectedTags();
		
		if (transform_future.IsReady())
		{
			auto [camera_transform, local_tag_transforms] = transform_future.Get();
			if (!camera_transform.Equals(FTransform::Identity))
			{
				camera->next_update_time = last_now + static_cast<int64_t>(camera->UpdateTransform(camera_transform) * 1e9);
				for (auto [tag, local_transform] : local_tag_transforms)
				{
					FMatrix world_transform = (local_transform * FQuat::MakeFromEuler(FVector(0, 0, 90)).ToMatrix() * FQuat::MakeFromEuler(FVector(0, 90, 0)).ToMatrix()) * camera->camera_transform.ToMatrixNoScale();
					camera->next_update_time = min(	camera->next_update_time,
													last_now + static_cast<int64_t>(tag->UpdateTransform(FTransform(world_transform)) * tag->update_rate * 1e9));

				}
			}
			transform_future = TFuture<pair<FTransform, map<ATag*, FMatrix>>>();
		}

		if (!transform_future.IsValid() && last_now >= camera->next_update_time)
			transform_future = Async(EAsyncExecution::Thread, [camera] { return camera->UpdateTags(camera->cv_frame); });
		last_now = chrono::high_resolution_clock::now().time_since_epoch().count();
	}
	if (transform_future.IsValid())
		transform_future.Wait();

	camera->in_use = false;
	
	camera->destroy_lock.unlock();
}

uint32 CameraManager::Run()
{
	TArray<ATrackingCamera*> cameras = ball->tracking_cameras;

	vector<deque<Detection>> ball_2d_detections(cameras.Num());


	for (int i = 0; i < cameras.Num(); i++)
		camera_threads.push_back(Async(EAsyncExecution::Thread, [&, i, cameras]
		{
			CameraLoop(cameras[i], &ball_2d_detections[i]);
		}));

	while (run_thread)
	{
		usleep(10000);
		
		auto time_before = chrono::high_resolution_clock::now();
		
		vector<Point2d> ball_points;

		ball_detection_2d_mut.lock();
		auto ball_det_backup = ball_2d_detections;
		ball_detection_2d_mut.unlock();

		double time = 1e20;

		for (int i = 0; i < cameras.Num(); i++)
			if (ball_det_backup[i].size() && cameras[i]->debug_output)
				UE_LOG(LogTemp, Warning, TEXT("Last frame time of %s: %f"), *cameras[i]->camera_path,
			       ball_det_backup[i].back().time);

		for (auto& ball_position : ball_det_backup)
			if (ball_position.size())
				time = min(time, ball_position.back().time);

		for (int i = 0; i < cameras.Num(); i++)
		{
			if (ball_det_backup[i].size() < 2)
				ball_points.push_back({-1, -1});
			else
			{
				int j = 0;
				while (j < ball_det_backup[i].size() - 1 && ball_det_backup[i][j].time <= time)
					j++;

				if (j == 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Something seems wrong with the cameras, they seem very desyncronized! %d"), i);
					
					ball_points.push_back({-1, -1});
					continue;
				}

				Vec2d velocity = ball_det_backup[i][j].position
					- ball_det_backup[i][j - 1].position;

				velocity /= ball_det_backup[i][j].time
					- ball_det_backup[i][j - 1].time;

				Vec2d current_position = Vec2d(ball_det_backup[i][j - 1].position)
					+ velocity * (time - ball_det_backup[i][j - 1].time);
				ball_points.push_back(current_position);
			}
		}
		// compute triangulation of points
		vector<Mat> projection_matrices;
		for (ATrackingCamera* camera : cameras)
		{
			Mat RT = ConvertToCameraMatrix(camera->camera_transform);
			Mat projection = camera->K() * RT;
			projection_matrices.push_back(projection);
		}

		Vec4d average_position(0, 0, 0, 0);
		double num = 0;
		
		for (int i = 0; i < cameras.Num(); i++)
			for (int j = i + 1; j < cameras.Num(); j++)
			{
				if (ball_points[i] == Point2d{-1, -1} || ball_points[j] == Point2d{-1, -1})
					continue;

				vector<Point2d> a{ball_points[i]}, b{ball_points[j]};
				Vec4d position;
				triangulatePoints(projection_matrices[i], projection_matrices[j], a, b, position);
				position /= position[3];

				Mat reprojection = projection_matrices[i] * position;
				Vec3d reprojected_ball_point = Vec3d((double*)reprojection.data);
				reprojected_ball_point /= reprojected_ball_point[2];

				double error = pow(ball_points[i].x - reprojected_ball_point[0], 2) + pow(ball_points[i].y - reprojected_ball_point[1], 2);

				if (error > 200)
					continue;
				
				average_position += position;
				num++;
			}

		if (num > 0)
			average_position /= num;

		
		ball_position_mut.lock();
		{
			double last_frame = 0;
			for (auto camera : cameras)
				last_frame = max(last_frame, camera->last_frame_time);
			while (ball_positions.size() && abs(ball_positions.front().time - last_frame) > 1)
			{
				// no point in recording more than 1 second
				ball_positions.pop_front();
			}
		}
		ball_position_mut.unlock();
		
		if (num == 0)
		{
			ball->tracking_path = {};
			continue;
		}
		
		FVector p(average_position.val[2], average_position.val[0], -average_position.val[1]);
		ball->position = p;

		ball_position_mut.lock();
		{
			ball_positions.push_back({p, time});
		}
		ball_position_mut.unlock();
		
		if (ball_positions.size() >= 10)
		{
			auto last = ball_positions.back();
			double diff = ((tracking_path(last.time) - last.position) / last.position.ComponentMax(tracking_path(last.time))).GetAbsMax();

			if (diff < 0.15)
			{
				ball_paths.push_back(tracking_path = ParabPath::fromNPoints({ball_positions.end() - min({++num_points_in_path, int(ball_positions.size()), 30}), ball_positions.end()}));
			}
			else
			{
				if (ball->save_paths && num_points_in_path >= 30 && ball_positions.size() >= 30) // save the path to file
				{
					auto now = std::chrono::system_clock::now();
					auto in_time_t = std::chrono::system_clock::to_time_t(now);

					std::stringstream ss;
					ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");

					string path = "/home/elias/Documents/ParabPaths/"+ ss.str() + ".txt";
					
					ofstream parab_file(path);

					auto t0 = ball_paths.begin()->t0;
					
					if (parab_file.is_open())
					{
						parab_file << "t, x = " + to_string(tracking_path.vx / 1e3) + " * x + " + to_string(tracking_path.px / 1e3) + "\n";
						for (ParabPath output_path : ball_paths)
						{
							output_path += (t0 - output_path.t0);
							parab_file << to_string(output_path.vx / 1e3) + " * x + " + to_string(output_path.px / 1e3) + "\n";
						}
						for (auto i = ball_positions.end() - min(num_points_in_path, int(ball_positions.size())); i < ball_positions.end(); ++i)
						{
							auto [position, t] = *i;
							parab_file << t-t0 << " " << position.X / 1e3 << "\n";
						}
						parab_file << "t, y = " + to_string(tracking_path.vy / 1e3) + " * t + " + to_string(tracking_path.py / 1e3) + "\n";
						for (ParabPath output_path : ball_paths)
						{
							output_path += (t0 - output_path.t0);
							parab_file << to_string(output_path.vy / 1e3) + " * x + " + to_string(output_path.py / 1e3) + "\n";
						}
						for (auto i = ball_positions.end() - min(num_points_in_path, int(ball_positions.size())); i < ball_positions.end(); ++i)
						{
							auto [position, t] = *i;
							parab_file << t-t0 << " " << position.Y / 1e3 << "\n";
						}
						parab_file << "t, z = " + to_string(tracking_path.a / 1e3) + " * t^2 + " + to_string(tracking_path.b / 1e3) + " * t + " + to_string(tracking_path.c / 1e3) + "\n";
						for (ParabPath output_path : ball_paths)
						{
							output_path += (t0 - output_path.t0);
							parab_file << to_string(output_path.a / 1e3) + " * x * x + " + to_string(output_path.b / 1e3) + " * x + " + to_string(output_path.c / 1e3) + "\n";
						}
						for (auto i = ball_positions.end() - min(num_points_in_path, int(ball_positions.size())); i < ball_positions.end(); ++i)
						{
							auto [position, t] = *i;
							parab_file << t-t0 << " " << position.Z / 1e3 << "\n";
						}
						parab_file.close();

						UE_LOG(LogTemp, Display, TEXT("Saved path to file: %s"), *FString(path.c_str()));
					} else
					{
						UE_LOG(LogTemp, Display, TEXT("Could not open file: %s"), *FString(path.c_str()));
					}
				}

				ball_paths.clear();
				ball_paths.push_back(tracking_path = ParabPath::fromNPoints({ball_positions.end() - 10, ball_positions.end()}));

				if (abs(tracking_path.derivative2() - ball->g) > 1500)
					tracking_path = {};

				num_points_in_path = 10;
			}
		}
		else
		{
			tracking_path = {};
		}
		
		ball->tracking_path = tracking_path;

		auto time_after = chrono::high_resolution_clock::now();

		UE_LOG(LogTemp, Display, TEXT("Took %f ms to update tracking path"), float((time_after-time_before).count()) / 1e6);
	}

	for (auto& f : camera_threads) // wait for all threads to stop
		f.Wait();

	thread_stopped = true;

	return 0;
}

void CameraManager::Stop()
{
	run_thread = false;

	while (!thread_stopped) { usleep(1); }

	FRunnable::Stop();
}

void CameraManager::DrawBallHistory()
{
	vector<ParabPath> paths;
	ball_position_mut.lock();
	auto positions = ball_positions; // make a copy of the data so the camera thread is not held up too much
	ball_position_mut.unlock();

	if (positions.size() == 0)
		return;


	for (int i = 0; i < int(positions.size()) - 1; i++)
	{
		DrawDebugLine(ball->GetWorld(), positions[i].position, positions[i + 1].position, FColor::Green, false, -1, 0, 10);
	}

	if (tracking_path.t0 != -1)
	{
		FColor color = FColor::Red;
		color.R = min(255 * 100 / abs(tracking_path.derivative2() - ball->g), 255.);
		//tracking_path.Draw(ball->GetWorld(), color, 20, 1, -1);

		if (abs(tracking_path.derivative2() - ball->g) < 1500)
		{
			tracking_path.Draw(ball->GetWorld(), FColor::Blue, 20, 1, -1);

			auto future_path = tracking_path + (tracking_path.t1 - tracking_path.t0);
			future_path.t1 = future_path.t0 + 1;

			future_path.Draw(ball->GetWorld(), FColor::Red, 15, 1, -1);
		}
	}

	//stringstream ss; 
	// for (int i = 0; i < int(positions.size()); i++)
	// {
	// 	ss << "{{" << positions[i].position.X << ", " << positions[i].position.Y << ", " << positions[i].position.Z << "}," << positions[i].time-t0 << "},\n";
	// }
	// ss << "\n\n\n";
	//
	// FString msg = ss.str().c_str();
	//
	// UE_LOG(LogTemp, Log, TEXT("%s"), *msg);
}

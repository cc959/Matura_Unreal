// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraManager.h"


#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <deque>

#include "EventPasser.h"

#include "GlobalIncludes.h"


CameraManager::CameraManager(class ABall* ball) : ball(ball)
{
	Thread = FRunnableThread::Create(this, TEXT("Camera Thread"));
}

CameraManager::~CameraManager()
{
	Stahp();

	if (Thread)
		delete Thread;
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

void CameraManager::CameraLoop(ATrackingCamera* camera, int camera_id)
{
	TFuture<std::pair<FTransform, std::map<ATag*, FMatrix>>> transform_future;
	int64_t last_now = std::chrono::high_resolution_clock::now().time_since_epoch().count();


	while (!camera || !camera->loaded)
	{
		if (!run_threads)
			return;
		LogError(TEXT("%s is not ready yet or null"), *camera->camera_path)
		usleep(10000); //wait 10 ms
	}

	camera->destroy_lock.lock();

	camera->must_update_tags = true;
	while (camera->must_update_tags)
		usleep(10000); //wait 10 ms

	LogError(TEXT("%s loop has started"), *camera->camera_path);

	camera->in_use = true;

	while (run_threads)
	{
		// camera wants to be released, but is waiting for this thread to finish
		if (!camera->loaded)
		{
			if (transform_future.IsValid())
				transform_future.Wait();
			LogError(TEXT("%s is not loaded anymore, exiting thread"), *camera->camera_path);
			camera->destroy_lock.unlock();
			return;
		}

		double time = camera->SyncFrame() / 1000.;
		camera->GetFrame();

		Point2d ball_position = camera->FindBall();

		camera->last_frame_time = time;
		
		if (event_passer.push({ball_position, time, camera_id}))
		{
			LogWarning(TEXT("Dropped camera event on camera %s"), *camera->camera_path);
		}

		camera->DrawDetectedTags();

		if (transform_future.IsReady())
		{
			auto [camera_transform, local_tag_transforms] = transform_future.Get();
			if (!camera_transform.Equals(FTransform::Identity))
			{
				camera->next_update_time = last_now + static_cast<int64_t>(camera->UpdateTransform(camera_transform) * 1e9);
				for (auto [tag, local_transform] : local_tag_transforms)
				{
					FMatrix world_transform = (local_transform * FQuat::MakeFromEuler(FVector(0, 0, 90)).ToMatrix() *
						FQuat::MakeFromEuler(FVector(0, 90, 0)).ToMatrix()) * camera->camera_transform.ToMatrixNoScale();
					camera->next_update_time = min(camera->next_update_time,
					                               last_now + static_cast<int64_t>(tag->UpdateTransform(FTransform(world_transform)) * tag->
						                               update_rate * 1e9));
				}
			}
			transform_future = TFuture<std::pair<FTransform, std::map<ATag*, FMatrix>>>();
		}

		if (!transform_future.IsValid() && last_now >= camera->next_update_time)
			transform_future = Async(EAsyncExecution::Thread, [camera] { return camera->UpdateTags(camera->cv_frame); });
		last_now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	}
	if (transform_future.IsValid())
		transform_future.Wait();

	camera->in_use = false;

	camera->destroy_lock.unlock();
}

void CameraManager::Stahp()
{
	run_threads = false;
	event_passer.stop();

	Thread->WaitForCompletion();
}

uint32 CameraManager::Run()
{
	TArray<ATrackingCamera*> cameras = ball->tracking_cameras;

	std::vector<std::deque<Detection>> ball_2d_detections(cameras.Num());
	
	for (int i = 0; i < cameras.Num(); i++)
		camera_threads.push_back(Async(EAsyncExecution::Thread, [&, i, cameras]
		{
			CameraLoop(cameras[i], i);
		}));

	Detection det;
	while (event_passer.pop(&det))
	{
		auto time_before = std::chrono::high_resolution_clock::now();

		if (det.position == Point2d{-1, -1})
		{
			ball->tracking_path.push({});
			ball->started = true;
			continue;
		}
		
		ball_2d_detections[det.camera_id].push_back(det);

		while (ball_2d_detections[det.camera_id].size() && abs(
			ball_2d_detections[det.camera_id].front().time - ball_2d_detections[det.camera_id].back().time) > 0.1)
			ball_2d_detections[det.camera_id].pop_front();

		std::vector<Point2d> ball_points;

		double time = 1e20;

		for (int i = 0; i < cameras.Num(); i++)
			if (ball_2d_detections[i].size() && cameras[i]->debug_output)
				LogWarning(TEXT("Last frame time of %s: %f"), *cameras[i]->camera_path,
			           ball_2d_detections[i].back().time);

		for (auto& ball_position : ball_2d_detections)
			if (ball_position.size())
				time = min(time, ball_position.back().time);

		for (int i = 0; i < cameras.Num(); i++)
		{
			if (ball_2d_detections[i].size() < 2)
				ball_points.push_back({-1, -1});
			else
			{
				int j = 0;
				while (j < ball_2d_detections[i].size() - 1 && ball_2d_detections[i][j].time <= time)
					j++;

				if (j == 0)
				{
					ball_points.push_back({-1, -1});
					continue;
				}

				Vec2d velocity = ball_2d_detections[i][j].position
					- ball_2d_detections[i][j - 1].position;

				velocity /= ball_2d_detections[i][j].time
					- ball_2d_detections[i][j - 1].time;

				Vec2d current_position = Vec2d(ball_2d_detections[i][j - 1].position)
					+ velocity * (time - ball_2d_detections[i][j - 1].time);
				ball_points.push_back(current_position);
			}
		}
		// compute triangulation of points
		std::vector<Mat> projection_matrices;
		for (ATrackingCamera* camera : cameras)
		{
			Mat RT = ConvertToCameraMatrix(camera->camera_transform);
			Mat projection = camera->K() * RT;
			projection_matrices.push_back(projection);
		}

		for (int i = 0; i < cameras.Num(); i++)
			cameras[i]->used_ball = ball_points[i];
		
		Vec4d average_position(0, 0, 0, 0);
		double num = 0;

		for (int i = 0; i < cameras.Num(); i++)
			for (int j = i + 1; j < cameras.Num(); j++)
			{
				if (ball_points[i] == Point2d{-1, -1} || ball_points[j] == Point2d{-1, -1})
					continue;

				std::vector<Point2d> a{ball_points[i]}, b{ball_points[j]};
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
		else
		{
			ball->tracking_path.push({});
			ball->started = true;
			continue;
		}

		double last_frame = 0;
		for (auto camera : cameras)
			last_frame = max(last_frame, camera->last_frame_time);
		while (ball_positions.size() && abs(ball_positions.front().time - last_frame) > 1)
		{
			// no point in recording more than 1 second
			ball_positions.pop_front();
		}

		if (num == 0)
		{
			ball->tracking_path.push({});
			ball->started = true;
			continue;
		}

		FVector p(average_position.val[2], average_position.val[0], -average_position.val[1]);
		ball->position = p;

		ball_positions.push_back({p, time});

		if (ball_positions.size() >= 10)
		{
			auto last = ball_positions.back();
			double diff = ((tracking_path(last.time) - last.position) / last.position.ComponentMax(tracking_path(last.time))).GetAbsMax();

			if (diff < 0.15)
			{
				ball_paths.push_back(tracking_path = ParabPath::fromNPoints(
					std::vector(ball_positions.end() - min(++num_points_in_path, min(int(ball_positions.size()), 30)), ball_positions.end())));
			}
			else
			{
				if (ball->save_paths && num_points_in_path >= 30 && ball_positions.size() >= 30) // save the path to file
				{
					auto now = std::chrono::system_clock::now();
					auto in_time_t = std::chrono::system_clock::to_time_t(now);

					std::stringstream ss;
					ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");

					std::string path = "/home/elias/Documents/ParabPaths/" + ss.str() + ".txt";

					std::ofstream parab_file(path);

					auto t0 = ball_paths.begin()->t0;

					if (parab_file.is_open())
					{
						parab_file << "t, x = " + std::to_string(tracking_path.vx / 1e3) + " * x + " + std::to_string(tracking_path.px / 1e3) + "\n";
						for (ParabPath output_path : ball_paths)
						{
							output_path += (t0 - output_path.t0);
							parab_file << std::to_string(output_path.vx / 1e3) + " * x + " + std::to_string(output_path.px / 1e3) + "\n";
						}
						for (auto i = ball_positions.end() - min(num_points_in_path, int(ball_positions.size())); i < ball_positions.end(); ++i)
						{
							auto [position, t] = *i;
							parab_file << t - t0 << " " << position.X / 1e3 << "\n";
						}
						parab_file << "t, y = " + std::to_string(tracking_path.vy / 1e3) + " * t + " + std::to_string(tracking_path.py / 1e3) + "\n";
						for (ParabPath output_path : ball_paths)
						{
							output_path += (t0 - output_path.t0);
							parab_file << std::to_string(output_path.vy / 1e3) + " * x + " + std::to_string(output_path.py / 1e3) + "\n";
						}
						for (auto i = ball_positions.end() - min(num_points_in_path, int(ball_positions.size())); i < ball_positions.end(); ++i)
						{
							auto [position, t] = *i;
							parab_file << t - t0 << " " << position.Y / 1e3 << "\n";
						}
						parab_file << "t, z = " + std::to_string(tracking_path.a / 1e3) + " * t^2 + " + std::to_string(tracking_path.b / 1e3) +
							" * t + " + std::to_string(tracking_path.c / 1e3) + "\n";
						for (ParabPath output_path : ball_paths)
						{
							output_path += (t0 - output_path.t0);
							parab_file << std::to_string(output_path.a / 1e3) + " * x * x + " + std::to_string(output_path.b / 1e3) + " * x + " +
								std::to_string(output_path.c / 1e3) + "\n";
						}
						for (auto i = ball_positions.end() - min(num_points_in_path, int(ball_positions.size())); i < ball_positions.end(); ++i)
						{
							auto [position, t] = *i;
							parab_file << t - t0 << " " << position.Z / 1e3 << "\n";
						}
						parab_file.close();

						LogDisplay(TEXT("Saved path to file: %s"), *FString(path.c_str()));
					}
					else
					{
						LogDisplay(TEXT("Could not open file: %s"), *FString(path.c_str()));
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

		ball->tracking_path.push(tracking_path);
		ball->started = true;

		auto time_after = std::chrono::high_resolution_clock::now();
	}

	for (auto& f : camera_threads) // wait for all threads to stop
		f.Wait();

	ball->tracking_path.stop();
	
	return 0;
}

void CameraManager::Stop()
{
	Stahp();

	FRunnable::Stop();
}

void CameraManager::DrawBallHistory()
{
	std::vector<ParabPath> paths;

	auto positions = ball_positions; // make a copy of the data so the camera thread is not held up too much

	if (positions.size() == 0)
		return;


	for (int i = 0; i < int(positions.size()) - 1; i++)
	{
		DrawDebugLine(ball->GetWorld(), positions[i].position, positions[i + 1].position, FColor::Green, false, -1, 0, 10);
	}

	if (tracking_path.t0 != -1)
	{
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
	// LogDisplay(TEXT("%s"), *msg);
}

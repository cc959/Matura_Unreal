// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraManager.h"

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

void CameraManager::CameraLoop(ATrackingCamera* camera, deque<Detection>* past_ball_positions)
{
	int frame_id = 0;
	while (run_thread)
	{
		if (!camera || !camera->finished_init)
		{
			UE_LOG(LogTemp, Error, TEXT("%s is not ready yet or null"), *camera->camera_path)
			usleep(10000); //wait 10 ms
			continue;
		}

		double time = camera->SyncFrame();
		camera->GetFrame();

		Point2d ball_position = camera->FindBall();

		ball_position_mut.lock();
		while (past_ball_positions->size() && past_ball_positions->front().time < time - 1000)
			past_ball_positions->pop_front();

		if (ball_position != Point2d{-1, -1})
			past_ball_positions->push_back({ball_position, time});
		ball_position_mut.unlock();
		
		if (frame_id++ % 10 == 0)
		{
			FTransform camera_transform = camera->LocalizeCamera();
			if (!camera_transform.Equals(FTransform::Identity))
				camera->april_transform = camera_transform;
		}
	}
}

uint32 CameraManager::Run()
{
	TArray<ATrackingCamera*> cameras = ball->tracking_cameras;

	vector<deque<Detection>> past_ball_positions(cameras.Num());
	
	for (int i = 0; i < cameras.Num(); i++) 
		camera_threads.push_back(Async(EAsyncExecution::Thread, [&, i, cameras]
		{
			CameraLoop(cameras[i], &past_ball_positions[i]);
		}));
	
	while (run_thread)
	{
		usleep(10000);
		vector<Point2d> ball_points;

		ball_position_mut.lock();
		auto ball_pos_backup = past_ball_positions;
		ball_position_mut.unlock();
		
		double time = 1e20;

		for (int i = 0; i < cameras.Num(); i++)
			if (ball_pos_backup[i].size())
				UE_LOG(LogTemp, Warning, TEXT("Last frame time of %s: %f"), *cameras[i]->camera_path, ball_pos_backup[i].back().time);
		
		for (auto& ball_position : ball_pos_backup)
			if (ball_position.size())
				time = min(time, ball_position.back().time);
		
		for (int i = 0; i < cameras.Num(); i++)
		{
			if (ball_pos_backup[i].size() < 2)
				ball_points.push_back({-1, -1});
			else
			{
				int j = 0;
				while (j < ball_pos_backup[i].size() - 1 && ball_pos_backup[i][j].time < time)
					j++;

				if (j == 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Something seems wrong with the cameras, they seem very desyncronized!"));
					ball_points.push_back({-1, -1});
					continue;
				}
				
				Vec2d velocity = ball_pos_backup[i][j].position
					- ball_pos_backup[i][j-1].position;

				velocity /= past_ball_positions[i][j].time
					- ball_pos_backup[i][j-1].time;

				Vec2d current_position = Vec2d(ball_pos_backup[i][j-1].position)
					+ velocity * (time - ball_pos_backup[i][j-1].time);
				ball_points.push_back(current_position);
			}
		}
		// compute triangulation of points
		vector<Mat> projection_matrices;
		for (ATrackingCamera* camera : cameras)
		{
			Mat RT = ConvertToCameraMatrix(camera->april_transform);
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

				average_position = ((average_position * num) + position) / (num + 1);
				num++;
			}

		FVector p(average_position.val[2], average_position.val[0], -average_position.val[1]);
		ball->position = p;
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

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

struct Detection
{
	Point2d position;
	double time;
};

uint32 CameraManager::Run()
{
	int frame_id = 0;

	vector<deque<Detection>> past_ball_positions;

	while (run_thread)
	{
	start:
		// make copy of cameras used in case updated in editor while processing
		TArray<ATrackingCamera*> cameras = ball->tracking_cameras;

		for (ATrackingCamera* camera : cameras)
			if (!camera || !camera->finished_init)
			{
				UE_LOG(LogTemp, Error, TEXT("At least one camera is not ready yet or null"))
				usleep(10000); //wait 10 ms
				goto start;
			}

		if (past_ball_positions.size() != cameras.Num())
			past_ball_positions = vector(cameras.Num(), deque<Detection>());

		auto time_start = chrono::high_resolution_clock::now();

		vector<TFuture<double>> sync_threads;
		for (ATrackingCamera* camera : cameras)
			sync_threads.push_back(Async(EAsyncExecution::Thread, [camera] { return camera->SyncFrame(); }));

		vector<double> cap_times;

		for (auto& future : sync_threads)
			cap_times.push_back(future.Get());

		UE_LOG(LogTemp, Warning, TEXT("Capture times:\n%s: %f\n%s: %f"), *cameras[0]->camera_path, cap_times[0],
		       *cameras[1]->camera_path, cap_times[1]);

		vector<TFuture<void>> processing_threads;

		// decompress frames in parallel
		for (ATrackingCamera* camera : cameras)
			processing_threads.push_back(Async(EAsyncExecution::Thread, [camera] { camera->GetFrame(); }));

		// make sure they are all done
		for (auto& future : processing_threads)
			future.Wait();

		vector<TFuture<Point2d>> detection_threads;
		// decompress frames in parallel
		for (ATrackingCamera* camera : cameras)
			detection_threads.push_back(Async(EAsyncExecution::Thread, [camera] { return camera->FindBall(); }));

		for (int i = 0; i < cameras.Num(); i++)
		{
			auto ball_point = detection_threads[i].Get();

			if (ball_point != Point2d{-1, -1})
				past_ball_positions[i].push_back({ball_point, cap_times[i]});
		}

		vector<Point2d> ball_points; // TODO: Implement parabolic predictions instead of linear

		UE_LOG(LogTemp, Error, TEXT("send help %d"), past_ball_positions.size());


		for (int i = 0; i < cameras.Num(); i++)
		{
			double time = *max_element(cap_times.begin(), cap_times.end());


			while (past_ball_positions[i].size() && past_ball_positions[i].front().time < time - 200.)
				// discard points that are more than a two tenths of a second old
				past_ball_positions[i].pop_front();

			if (past_ball_positions[i].size() < 2)
				ball_points.push_back({-1, -1});
			else
			{
				Vec2d velocity = past_ball_positions[i].back().position
					- past_ball_positions[i][past_ball_positions[i].size() - 2].position;

				velocity /= past_ball_positions[i].back().time
					- past_ball_positions[i][past_ball_positions[i].size() - 2].time;

				Vec2d current_position = Vec2d(past_ball_positions[i].back().position)
					+ velocity * (time - past_ball_positions[i].back().time);
				ball_points.push_back(current_position);
			}
		}

		if (frame_id++ % 10 == 0) // every 10 frames, look at apriltag
			for (ATrackingCamera* camera : cameras)
			{
				Async(EAsyncExecution::Thread, [camera]()
				{
					FTransform transform = camera->LocalizeCamera();
					if (!transform.Equals(FTransform::Identity))
					{
						camera->april_transform = transform;
					}
				});
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

		auto time_end = chrono::high_resolution_clock::now();

		UE_LOG(LogTemp, Warning, TEXT("One tracking run took %f ms -> %f fps"), (time_end - time_start).count() / 1e6,
		       1e9 / (time_end - time_start).count());
	}
	thread_stopped = true;

	return 0;
}

void CameraManager::Stop()
{
	run_thread = false;

	while (!thread_stopped) { usleep(1); }

	FRunnable::Stop();
}

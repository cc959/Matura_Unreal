// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraManager.h"

#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <deque>

#include "EventPasser.h"

#include "GlobalIncludes.h"

using namespace TCPMessages;

CameraManager::CameraManager(vector<ATrackingCamera *> cameras) : cameras(cameras) {}

void CameraManager::Start()
{
	for (auto camera : cameras)
	{
		camera->InitCamera();
	}

	int64_t last_frame_time = NOW_NS;

	int num_frames = 0;

	while (run_threads)
	{
		// for (auto camera : cameras)
		// camera->SyncFrame() / 1000.;

		for (auto camera : cameras)
			camera->GetFrame();

		for (auto camera : cameras)
			camera->FindBall();

		if (num_frames >= 200)
		{
			int64_t time_since_last_frame = NOW_NS - last_frame_time;
			LogDisplay("Fps %f", 1e9 / double(time_since_last_frame) * double(num_frames));
			last_frame_time = NOW_NS;
			num_frames = 0;
		}
		num_frames++;
	}

	return;

	for (int i = 0; i < cameras.size(); i++)
	{
		camera_threads.push_back(thread(&CameraManager::CameraLoop, this, cameras[i], i));
	}
}

void CameraManager::CameraLoop(ATrackingCamera *camera, int camera_id)
{
	// camera->InitCamera();

	std::future<void> tag_future;

	int64_t last_april_tag_update = NOW_NS;
	int64_t last_debug_frame_update = NOW_NS;
	int64_t last_frame_time = NOW_NS;
	int num_frames = 0;

	while (!camera->loaded)
	{
		if (!run_threads)
			return;
		LogErr(TEXT("%s is not ready yet or null"), camera->camera_path.c_str());

		usleep(10000); // wait 10 ms
	}

	camera->destroy_lock.lock();

	LogErr(TEXT("%s loop has started"), camera->camera_path.c_str());

	camera->in_use = true;

	while (run_threads)
	{
		// camera wants to be released, but is waiting for this thread to finish
		if (!camera->loaded)
		{
			if (tag_future.valid())
				tag_future.wait();
			LogErr(TEXT("%s is not loaded anymore, exiting thread"), camera->camera_path.c_str());
			camera->destroy_lock.unlock();
			return;
		}

		double time = camera->SyncFrame() / 1000.;
		camera->GetFrame();

		Point2d ball_position = camera->FindBall();

		camera->last_frame_time = time;

		if (event_passer.push(_Detection{std::make_pair(ball_position.x, ball_position.y), time, camera_id}))
		{
			// LogWarning(TEXT("Dropped camera event on camera %s"), camera->camera_path.c_str());
		}

		if (camera->debug_frame_type != _DebugFrameType::None && NOW_NS - last_debug_frame_update > 1e9 / 20.0) // 30 fps
		{
			debug_frame_passer.push({camera_id, camera->cv_debug_frame.clone()});
			last_debug_frame_update = NOW_NS;
		}

		if (!tag_future.valid() || tag_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready && NOW_NS - last_april_tag_update > 1e9 * 3.0) // 1 second
		{
			tag_future = std::async(std::launch::async, [this, camera_id, camera]
									{ 
										auto tags = camera->UpdateTags(camera->cv_frame.clone());
										tag_event_passer.push({camera_id, tags});
										LogDisplay("Tags updated for camera %s", camera->camera_path.c_str()); });

			last_april_tag_update = NOW_NS;
		}
		if (num_frames >= 200)
		{
			int64_t time_since_last_frame = NOW_NS - last_frame_time;
			LogDisplay("Fps for camera %s: %f", camera->camera_path.c_str(), 1e9 / double(time_since_last_frame) * double(num_frames));
			last_frame_time = NOW_NS;
			num_frames = 0;
		}
		num_frames++;
	}
	if (tag_future.valid())
		tag_future.wait();

	camera->in_use = false;

	camera->destroy_lock.unlock();
}

CameraManager::~CameraManager()
{
	Stop();
}

void CameraManager::Stop()
{
	run_threads = false;
	event_passer.stop();
	tag_event_passer.stop();
	debug_frame_passer.stop();

	for (auto &thread : camera_threads)
		thread.join();
}

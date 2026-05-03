// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include <deque>
#include <thread>
#include <future>

#include "EventPasser.h"
#include "TrackingCamera.h"
#include "TCPMessages.h"

using namespace TCPMessages;

class CameraManager
{
public:
	// Constructor, create the thread by calling this
	CameraManager(vector<ATrackingCamera *> cameras);

	// Destructor
	~CameraManager();

	void Start();
	void Stop();

	EventPasser<_Detection> event_passer;
	EventPasser<pair<int, vector<_TagDetection>>> tag_event_passer;
	EventPasser<pair<int, Mat>> debug_frame_passer;

private:
	void CameraLoop(ATrackingCamera *camera, int camera_id);

	vector<ATrackingCamera *> cameras;
	std::vector<std::thread> camera_threads;

	bool run_threads = true;
};

// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include <deque>

#include "Ball.h"
#include "EventPasser.h"
#include "MatrixTypes.h"
#include "TrackingCamera.h"

#include "ParabPath.h"

class MATURA_UNREAL_API CameraManager : public FRunnable
{
public:
	// Constructor, create the thread by calling this
	CameraManager(class ABall* ball);

	// Destructor
	virtual ~CameraManager() override;


	// Overriden from FRunnable
	// Do not call these functions yourself, that will happen automatically
	// Do your setup here, allocate memory, ect.
	virtual bool Init() override;
	// Main data processing happens here
	virtual uint32 Run() override;
	// Clean up any memory you allocated here
	virtual void Stop() override;

	void DrawBallHistory();

	struct Detection
	{
		Point2d position;
		double time;
		int camera_id;
	};

private:

	EventPasser<Detection> event_passer;
	
	std::deque<Position> ball_positions;
	std::deque<ParabPath> ball_paths;
	ParabPath tracking_path = {};
	int num_points_in_path;
	
	void CameraLoop(ATrackingCamera* camera, int camera_id);
	std::vector<TFuture<void>> camera_threads;

	// Thread handle. Control the thread using this, with operators like Kill and Suspend
	FRunnableThread* Thread;

	// Used to know when the thread should exit, changed in Stop(), read in Run()
	bool run_threads = true;

	class ABall* ball;

	void Stahp();
};

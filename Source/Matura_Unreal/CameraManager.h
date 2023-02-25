// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include <deque>

#include "Ball.h"
#include "MatrixTypes.h"
#include "TrackingCamera.h"

#include "ParabPath.h"


using namespace std;

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
	};

private:

	std::mutex ball_position_mut;
	std::deque<Position> ball_positions;
	std::deque<ParabPath> ball_paths;
	ParabPath tracking_path = {};
	int num_points_in_path;
	
	std::mutex ball_detection_2d_mut;
	void CameraLoop(ATrackingCamera* camera, std::deque<Detection>* ball_2d_detections);
	std::vector<TFuture<void>> camera_threads;

	// Thread handle. Control the thread using this, with operators like Kill and Suspend
	FRunnableThread* Thread;

	// Used to know when the thread should exit, changed in Stop(), read in Run()
	bool run_thread = true;
	bool thread_stopped = false;

	class ABall* ball;
};

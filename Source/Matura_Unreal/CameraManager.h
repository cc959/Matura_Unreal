// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include <deque>

#include "Ball.h"
#include "TrackingCamera.h"


class MATURA_UNREAL_API CameraManager : public FRunnable
{
public:

	// Constructor, create the thread by calling this
	CameraManager(class ABall* ball);

	// Destructor
	virtual ~CameraManager() override;


	// Overriden from FRunnable
	// Do not call these functions youself, that will happen automatically
	// Do your setup here, allocate memory, ect.
	virtual bool Init() override;
	// Main data processing happens here
	virtual uint32 Run() override;
	// Clean up any memory you allocated here
	virtual void Stop() override;

private:

	struct Detection
	{
		Point2d position;
		double time;
	};

	std::mutex ball_position_mut;
	void CameraLoop(ATrackingCamera* camera, std::deque<Detection>* past_ball_positions);
	std::vector<TFuture<void>> camera_threads;

	// Thread handle. Control the thread using this, with operators like Kill and Suspend
	FRunnableThread* Thread;

	// Used to know when the thread should exit, changed in Stop(), read in Run()
	bool run_thread = true;
	bool thread_stopped = false;

	class ABall* ball;
};

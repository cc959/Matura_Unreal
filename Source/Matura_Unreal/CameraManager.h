// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tag.h"
#include "apriltags/apriltag.h"

#include "PreOpenCVHeaders.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "PostOpenCVHeaders.h"

using namespace cv;

class MATURA_UNREAL_API CameraManager : public FRunnable
{
public:

	// Constructor, create the thread by calling this
	CameraManager(TArray<ATag*> april_tags, int camera_id);

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

	void GetTexture(UTexture2D* camera_texture_2d);
	
	TArray<ATag *> april_tags;

	apriltag_detector *at_td;

	VideoCapture cv_cap;
	Size cv_size;
	
	FMatrix relative_transformation;
	FMatrix world_transformation;

	int camera_id;
private:

	Mat cv_frame;
	Mat cv_frame_gray;
	Mat cv_frame_display;
	void CameraTick();

	// Thread handle. Control the thread using this, with operators like Kill and Suspend
	FRunnableThread* Thread;

	// Used to know when the thread should exit, changed in Stop(), read in Run()
	bool bRunThread = true;
	bool bThreadStopped = false;
};

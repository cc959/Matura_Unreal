// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tag.h"
#include "apriltags/apriltag.h"
#include <vector>

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "PreOpenCVHeaders.h"
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/video.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgcodecs.hpp"
#include <opencv2/bgsegm.hpp>
#include "PostOpenCVHeaders.h"

#include "TrackingCamera.h"

using namespace cv;

class MATURA_UNREAL_API CameraManager : public FRunnable
{
public:

	// Constructor, create the thread by calling this
	CameraManager(TArray<ATag*> april_tags, FString camera_path, class ATrackingCamera* camera);

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
	
	class VideoCapture cv_cap;
	Size cv_size;
	
	FTransform world_transform;

	FString camera_path;
private:
	class ATrackingCamera* camera;
	
	Mat cv_display;
	Ptr<BackgroundSubtractor> cv_bg_subtractor;
	Ptr<SimpleBlobDetector> cv_blob_detector;
	apriltag_detector* at_td;
	TArray<apriltag_family_t*> created_families;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper;


	std::vector<Point2f> prev;
	int blub = 0;


	bool DetectTags(const Mat& cv_frame, Mat& cv_frame_display, FTransform& world_transform);
	void DetectBlob(const Mat& cv_frame, Mat& cv_frame_display, int low_H, int low_S, int low_V, int high_H, int high_S, int high_V);
	void CameraTick();

	// Thread handle. Control the thread using this, with operators like Kill and Suspend
	FRunnableThread* Thread;

	// Used to know when the thread should exit, changed in Stop(), read in Run()
	bool bRunThread = true;
	bool bThreadStopped = false;
};

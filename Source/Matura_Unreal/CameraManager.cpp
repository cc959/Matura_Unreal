// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraManager.h"

#include <map>
#include <set>
#include <string>

#include "ToolContextInterfaces.h"
#include "apriltags/tag16h5.h"
#include "apriltags/tag25h9.h"
#include "apriltags/tag36h11.h"
#include "apriltags/tagCircle21h7.h"
#include "apriltags/tagCircle49h12.h"
#include "apriltags/tagCustom48h12.h"
#include "apriltags/tagStandard41h12.h"
#include "apriltags/tagStandard52h13.h"

using namespace std;

#include "apriltags/apriltag_pose.h"
#include "opencv2/imgproc.hpp"


CameraManager::CameraManager(TArray<ATag*> april_tags, int camera_id, class ATrackingCamera* camera) : april_tags(april_tags), camera_id(camera_id), camera(camera)
{
	Thread = FRunnableThread::Create(this, TEXT("Camera Thread"));
}

CameraManager::~CameraManager()
{
	if (Thread)
	{
		bRunThread = false;
		Thread->Kill();
		delete Thread;
	}
}

bool CameraManager::Init()
{
	cv_cap.open(camera_id, CAP_ANY);

	if (cv_cap.isOpened())
		cv_size = Size(cv_cap.get(CAP_PROP_FRAME_WIDTH), cv_cap.get(CAP_PROP_FRAME_HEIGHT));

	world_transform = FTransform::Identity;

	return FRunnable::Init();
}

uint32 CameraManager::Run()
{
	while (bRunThread)
	{
		CameraTick();
	}
	bThreadStopped = true;

	return 0;
}

void CameraManager::Stop()
{
	bRunThread = false;

	while (!bThreadStopped) { usleep(1); }

	cv_cap.release();

	FRunnable::Stop();
}

void CameraManager::GetTexture(UTexture2D* camera_texture_2d)
{
	if (!camera_texture_2d)
	{
		UE_LOG(LogTemp, Error, TEXT("camera texture pointer is null"));
		return;
	}

	if (cv_display.size().area() > 0)
	{
		void* texture_data = camera_texture_2d->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		memcpy(texture_data, cv_display.data, cv_display.elemSize() * cv_display.size().area());
		camera_texture_2d->GetPlatformData()->Mips[0].BulkData.Unlock();
		camera_texture_2d->UpdateResource();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CV_Frame is empty"));
	}
};

FTransform CameraManager::DetectTags(const Mat& cv_frame, Mat& cv_frame_display)
{
	apriltag_detector* at_td;
	at_td = apriltag_detector_create();
	at_td->quad_decimate = 1.0; // decimate factor
	at_td->quad_sigma = 0.0; // apply this much low-pass blur to input
	at_td->nthreads = 1; // use this many cpu threads
	at_td->debug = false; // print debug output
	at_td->refine_edges = true; // refine tag edges

	static const function<apriltag_family_t *()> family_functions[] = {
		tag16h5_create, tag25h9_create, tag36h11_create, tagCircle21h7_create, tagCircle49h12_create,
		tagCustom48h12_create, tagStandard41h12_create, tagStandard52h13_create
	};

	set<int> families;
	for (ATag* tag : april_tags)
		if (tag)
			families.insert(tag->tag_family);

	vector<apriltag_family_t*> created_families;

	for (int family : families)
	{
		apriltag_family_t* fam = family_functions[family]();
		apriltag_detector_add_family(at_td, fam);
		created_families.push_back(fam);
	}

	Mat cv_frame_gray;
	cvtColor(cv_frame, cv_frame_gray, COLOR_BGR2GRAY);

	// Make an image_u8_t header for the Mat data
	image_u8_t im = {
		.width = cv_frame_gray.cols,
		.height = cv_frame_gray.rows,
		.stride = cv_frame_gray.cols,
		.buf = cv_frame_gray.data
	};

	zarray_t* detections = apriltag_detector_detect(at_td, &im);

	FTransform average_transform = FTransform::Identity;
	float total_transformations = 0;

	// Draw detection outlines
	for (int i = 0; i < zarray_size(detections); i++)
	{
		std::map<string, int> family_names = {
			{"tag16h5", 0}, {"tag25h9", 1}, {"tag36h11", 2}, {"tagCircle21h7", 3}, {"tagCircle49h12", 4},
			{"tagStandard41h12", 5}, {"tagStandard52h13", 6}, {"tagCustom48h12", 7}
		};

		apriltag_detection_t* det;
		zarray_get(detections, i, &det);

		ATag* det_tag = NULL;
		for (ATag* tag : april_tags)
		{
			if (tag->tag_family == family_names[string(det->family->name)] && tag->tag_id == det->id)
				det_tag = tag;
		}

		if (det_tag)
		{
			apriltag_detection_info_t info;
			info.det = det;
			info.tagsize = 100;
			info.fx = 522.12285297286087;
			info.fy = 531.89050725948039;
			info.cx = 320;
			info.cy = 240;

			apriltag_pose_t pose;
			estimate_tag_pose(&info, &pose);

			FMatrix rotation_matrix(FVector(pose.R->data[0], pose.R->data[3], pose.R->data[6]),
			                        FVector(pose.R->data[1], pose.R->data[4], pose.R->data[7]),
			                        FVector(pose.R->data[2], pose.R->data[5], pose.R->data[8]),
			                        FVector(0));

			FMatrix relative_transformation = FQuat::MakeFromEuler(
					rotation_matrix.ToQuat().Euler() * FVector(-1, -1, 1)).
				ToMatrix();
			relative_transformation.SetOrigin(FVector(pose.t->data[0], pose.t->data[1], -pose.t->data[2]));

			FMatrix tag_transformation = det_tag->mesh->GetRelativeTransform().ToMatrixWithScale();
			FMatrix world_transformation = FQuat::MakeFromEuler(FVector(0, -90, -90)).ToMatrix() *
				relative_transformation.Inverse() * tag_transformation;

			average_transform.Blend(average_transform, FTransform(world_transformation),
			                        1 / (total_transformations + 1));
			total_transformations++;
		}

		line(cv_frame_display, Point(det->p[0][0], det->p[0][1]),
		     Point(det->p[1][0], det->p[1][1]),
		     Scalar(0, 0, 0xff), 2);
		line(cv_frame_display, Point(det->p[0][0], det->p[0][1]),
		     Point(det->p[3][0], det->p[3][1]),
		     Scalar(0, 0xff, 0), 2);
		line(cv_frame_display, Point(det->p[1][0], det->p[1][1]),
		     Point(det->p[2][0], det->p[2][1]),
		     Scalar(0xff, 0, 0), 2);
		line(cv_frame_display, Point(det->p[2][0], det->p[2][1]),
		     Point(det->p[3][0], det->p[3][1]),
		     Scalar(0xff, 0, 0), 2);

		string text = to_string(det->id);
		int fontface = FONT_HERSHEY_SCRIPT_SIMPLEX;
		double fontscale = 1.0;
		int baseline;
		Size textsize = getTextSize(text, fontface, fontscale, 2,
		                            &baseline);
		putText(cv_frame_display, text, Point(det->c[0] - textsize.width / 2, det->c[1] + textsize.height / 2),
		        fontface, fontscale, Scalar(0xff, 0x99, 0), 2);
	}

	// destroy tag families
	if (at_td)
	{
		apriltag_detector_destroy(at_td);
	}
	
	for (apriltag_family_t* tf : created_families)
	{
		if (!tf)
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not destroy tag family"));
			continue;
		}

		UE_LOG(LogTemp, Display, TEXT("Deleted %s"), *FString(tf->name));

		if (!strcmp(tf->name, "tag36h11"))
		{
			tag36h11_destroy(tf);
		}
		else if (!strcmp(tf->name, "tag25h9"))
		{
			tag25h9_destroy(tf);
		}
		else if (!strcmp(tf->name, "tag16h5"))
		{
			tag16h5_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagCircle21h7"))
		{
			tagCircle21h7_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagCircle49h12"))
		{
			tagCircle49h12_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagStandard41h12"))
		{
			tagStandard41h12_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagStandard52h13"))
		{
			tagStandard52h13_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagCustom48h12"))
		{
			tagCustom48h12_destroy(tf);
		}
	}

	return average_transform;
}

void CameraManager::DetectBlob(const Mat& cv_frame, Mat& cv_frame_display, int low_H, int low_S, int low_V, int high_H, int high_S, int high_V)
{
	Mat cv_frame_HSV, cv_frame_threshold;
	cvtColor(cv_frame, cv_frame_HSV, COLOR_RGB2HSV);
	
	inRange(cv_frame_HSV, Scalar(low_H, low_S, low_V), Scalar(high_H, high_S, high_V), cv_frame_threshold);

	SimpleBlobDetector::Params params;
	memset(&params, 0, sizeof(SimpleBlobDetector::Params));

	params.minThreshold = 250;
	params.maxThreshold = 256;
	params.thresholdStep = 1;
	params.minRepeatability = 1;

	params.filterByArea = true;
	params.minArea = 100;
	params.maxArea = 1000000;

	params.filterByColor = true;
	params.blobColor = 255;

	// params.filterByCircularity = true;
	// params.minCircularity = 0.3;
	// params.maxCircularity = 1;

	// params.filterByCircularity = true;
	// params.minCircularity = 0.8f;

	Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);
	std::vector<KeyPoint> points;
	detector->detect(cv_frame_threshold, points);

	cv_frame_display = cv_frame_threshold.clone();
	//drawKeypoints(cv_frame_threshold, points, cv_frame_threshold, Scalar(255, 0, 0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	drawKeypoints(cv_frame_display, points, cv_frame_display, Scalar(255, 0, 0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
}

void CameraManager::CameraTick()
{
	if (!cv_cap.isOpened())
		return;


	Mat cv_frame, cv_frame_display;
	cv_cap >> cv_frame;
	cv_frame_display = cv_frame.clone();
	
	world_transform = DetectTags(cv_frame, cv_frame_display);
	DetectBlob(cv_frame, cv_frame_display, camera->low_H, camera->low_S, camera->low_V, camera->high_H, camera->high_S, camera->high_V);

	cvtColor(cv_frame_display, cv_display, COLOR_BGR2BGRA);
}

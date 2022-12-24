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
#include "apriltags/apriltag_pose.h"


// https://github.com/nothings/stb (Thanks for the suggestion ChatGPT)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

CameraManager::CameraManager(TArray<ATag*> april_tags, FString camera_path, class ATrackingCamera* camera) :
	april_tags(april_tags), camera_path(camera_path), camera(camera)
{
	Thread = FRunnableThread::Create(this, TEXT("Camera Thread"));

	setNumThreads(16); // speed

	cv_bg_subtractor = createBackgroundSubtractorMOG2();

	SimpleBlobDetector::Params cv_blob_params;
	memset(&cv_blob_params, 0, sizeof(SimpleBlobDetector::Params));

	cv_blob_params.minThreshold = 254;
	cv_blob_params.maxThreshold = 256;
	cv_blob_params.thresholdStep = 1;
	cv_blob_params.minRepeatability = 1;

	cv_blob_params.filterByConvexity = true;
	cv_blob_params.minConvexity = 0.6;
	cv_blob_params.maxConvexity = 1;

	cv_blob_params.filterByArea = true;
	cv_blob_params.minArea = 100;
	cv_blob_params.maxArea = 10000;


	cv_blob_params.filterByCircularity = false;
	cv_blob_params.minCircularity = 0.6;
	cv_blob_params.maxCircularity = 1;


	cv_blob_detector = SimpleBlobDetector::create(cv_blob_params);

	at_td = apriltag_detector_create();

	at_td->quad_decimate = 1.0; // decimate factor
	at_td->quad_sigma = 0.0; // apply this much low-pass blur to input
	at_td->nthreads = 8; // use this many cpu threads
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

	for (int family : families)
	{
		apriltag_family_t* fam = family_functions[family]();
		apriltag_detector_add_family(at_td, fam);
		created_families.Append({fam});
	}
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
	cv_cap.open(TCHAR_TO_UTF8(*camera_path));

	if (cv_cap.isOpened())
	{
		if (!cv_cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G')))
			UE_LOG(LogTemp, Warning, TEXT("Could not set MJPG format"));

		if (!cv_cap.set(CAP_PROP_CONVERT_RGB, 0))
			UE_LOG(LogTemp, Warning, TEXT("Could not set raw format")); // UE OpenCV plugin broken, can't decompress jpeg, must do manually

		if (!cv_cap.set(CAP_PROP_FRAME_WIDTH, camera->resolution.X) || !cv_cap.set(
			CAP_PROP_FRAME_HEIGHT, camera->resolution.Y))
			UE_LOG(LogTemp, Warning, TEXT("Could not set size"));

		if (!cv_cap.set(CAP_PROP_FPS, 60))
			UE_LOG(LogTemp, Warning, TEXT("Could not set fps"));

		if (!cv_cap.set(CAP_PROP_AUTOFOCUS, 0))
			UE_LOG(LogTemp, Warning, TEXT("Could not set autofocus"));

		if (!cv_cap.set(CAP_PROP_FOCUS, 0))
			UE_LOG(LogTemp, Warning, TEXT("Could not set focus"));

		if (!cv_cap.set(CAP_PROP_AUTO_EXPOSURE, 1)) // this means no auto exposure, ¯\_(ツ)_/¯
			UE_LOG(LogTemp, Warning, TEXT("Could not turn off auto exposure"));

		if (!cv_cap.set(CAP_PROP_EXPOSURE, camera->exposure))
			UE_LOG(LogTemp, Warning, TEXT("Could not set exposure to %f"), camera->exposure)


		cv_size = Size(cv_cap.get(CAP_PROP_FRAME_WIDTH), cv_cap.get(CAP_PROP_FRAME_HEIGHT));

		UE_LOG(LogTemp, Warning, TEXT("Opened camera at %s with %dx%d at %d fps"), *camera_path,
		       int(cv_cap.get(CAP_PROP_FRAME_WIDTH)), int(cv_cap.get(CAP_PROP_FRAME_HEIGHT)),
		       int(cv_cap.get(CAP_PROP_FPS)));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open camera at path: %s"), *camera_path)
	}

	ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);

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

	FRunnable::Stop();
}

void CameraManager::GetTexture(UTexture2D* camera_texture_2d) // thread safety, what's that?
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

bool CameraManager::DetectTags(const Mat& cv_frame, Mat& cv_frame_display, FTransform& transform)
{
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
			info.tagsize = 100; // constant 1m (100cm) -> scaling is applied later on with the tag transformation
			info.fx = camera->focal_length.X;
			info.fy = camera->focal_length.Y;
			info.cx = cv_size.width / 2; // using half the resolution for now
			info.cy = cv_size.height / 2;

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

	transform = average_transform;

	return total_transformations > 0;
}

void CameraManager::DetectBlob(const Mat& cv_frame, Mat& cv_frame_display, int low_H, int low_S, int low_V, int high_H,
                               int high_S, int high_V)
{
	Mat cv_frame_HSV, cv_color_threshold, cv_bg_threshold, cv_threshold;
	cvtColor(cv_frame, cv_frame_HSV, COLOR_RGB2HSV);
	
	inRange(cv_frame_HSV, Scalar(low_H, low_S, low_V), Scalar(high_H, high_S, high_V), cv_color_threshold);

	auto time_start = std::chrono::high_resolution_clock::now().time_since_epoch();

	cv_bg_subtractor->apply(cv_frame, cv_bg_threshold, camera->learning_rate);

	auto time_end = std::chrono::high_resolution_clock::now().time_since_epoch();
	UE_LOG(LogTemp, Display, TEXT("Time to process background: %f ms, %f fps"),
	       double((time_end-time_start).count()) / 1e6, 1e9 / double((time_end-time_start).count()));

	bitwise_and(cv_color_threshold, cv_bg_threshold, cv_threshold);

	if (!cv_threshold.empty())
	{
		std::vector<KeyPoint> points;

		cv_blob_detector->detect(cv_threshold, points);

		if (camera->display_threshold_frame)
		{
			cvtColor(cv_threshold, cv_frame_display, COLOR_GRAY2BGR);
		}

		for (auto det : points)
		{
			const int radius = 50;
			circle(cv_frame_display, det.pt, radius, Scalar(0, 0, 255), 3);
			line(cv_frame_display, det.pt - Point2f(radius, 0), det.pt + Point2f(radius, 0), Scalar(0, 0, 255), 3);
			line(cv_frame_display, det.pt - Point2f(0, radius), det.pt + Point2f(0, radius), Scalar(0, 0, 255), 3);
			UE_LOG(LogTemp, Warning, TEXT("Detected!"));
		}

		if (points.size() == 0)
		{
			if (blub++ == 5)
				prev.clear();
		}
		else
		{
			blub = 0;
			prev.push_back(points[0].pt);
		}
		for (int i = 0; i < static_cast<int>(prev.size()) - 1; i++)
		{
			line(cv_frame_display, prev[i], prev[i + 1], Scalar(0, 255, 0), 3);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("threshold frame is empty!"));
	}
}

void CameraManager::CameraTick()
{
	if (!cv_cap.isOpened())
		return;

	Mat cv_frame_raw, cv_frame;
	cv_cap >> cv_frame_raw;

	auto time_start = std::chrono::high_resolution_clock::now().time_since_epoch();

	// uncompress jpeg frame with stb single header library -> OpenCV plugin for UE's uncompression is broken for some reason :(
	// this is also the reason why getting raw data from webcam instead of uncompressed -> setting CAP_PROP_CONVERT_RGB to false
	int Width, Height, NumComponents;
	uint8* UncompressedData = stbi_load_from_memory(cv_frame_raw.data,
	                                                cv_frame_raw.size().area() * cv_frame_raw.elemSize(),
	                                                &Width, &Height, &NumComponents, STBI_rgb);
	assert(Width == cv_size.width), assert(Height == cv_size.height), assert(NumComponents == 3);
	cv_frame = Mat(cv_size, CV_8UC3, UncompressedData);

	if (cv_frame.empty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Frame is empty"))
		return;
	}

	auto time_end = std::chrono::high_resolution_clock::now().time_since_epoch();

	UE_LOG(LogTemp, Display, TEXT("Time to uncompress frame: %f ms, %f fps"),
	       double((time_end-time_start).count()) / 1e6, 1e9 / double((time_end-time_start).count()));

	Mat camera_matrix = (cv::Mat_<double>(3, 3) <<
		camera->focal_length.X, 0, cv_size.width / 2,
		0, camera->focal_length.Y, cv_size.height / 2,
		0, 0, 1);

	// Create a vector of distortion coefficients
	Mat dist_coeffs = (cv::Mat_<double>(5, 1) << camera->k_twins.X, camera->k_twins.Y, camera->p_twins.X, camera->
		p_twins.Y, 0);

	Mat cv_frame_undistorted;
	undistort(cv_frame, cv_frame_undistorted, camera_matrix, dist_coeffs);

	Mat cv_frame_display = cv_frame_undistorted.clone();

	FTransform new_transform;
	if (DetectTags(cv_frame_undistorted, cv_frame_display, new_transform))
		world_transform = new_transform;


	DetectBlob(cv_frame_undistorted, cv_frame_display, camera->low_H, camera->low_S, camera->low_V, camera->high_H,
	           camera->high_S, camera->high_V);

	cvtColor(cv_frame_display, cv_display, COLOR_BGR2BGRA);

	stbi_image_free(UncompressedData); // free allocated data from cv_frame
}

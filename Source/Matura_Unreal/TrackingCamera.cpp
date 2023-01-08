// Fill out your copyright notice in the Description page of Project Settings.

#include "TrackingCamera.h"

#include <string>
#include <functional>
#include <set>
#include <map>
#include <thread>

// https://github.com/nothings/stb (Thanks for the suggestion ChatGPT)
#define STB_IMAGE_IMPLEMENTATION // make sure not to include implementation anywhere else
#include "stb_image.h"

#include "ujpeg.h"

using namespace std;


// Sets default values
ATrackingCamera::ATrackingCamera()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	scene_camera = CreateDefaultSubobject<UCameraComponent>(FName("Scene Camera"));
	scene_camera->SetAspectRatio(1);
	scene_camera->bConstrainAspectRatio = true;
	SetRootComponent(scene_camera);

	image_plate = CreateDefaultSubobject<UImagePlateComponent>(FName("Debug Image Plate"));
	image_plate->SetupAttachment(scene_camera);
	image_plate->SetRelativeLocation(FVector(11, 0, 0));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> plate_material(
		TEXT("/Script/Engine.Material'/Game/Unlit.Unlit'"));
	if (plate_material.Object)
		image_plate->SetMaterial(0, plate_material.Object);

	camera_mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Camera Mesh"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> camera(
		TEXT("/Script/Engine.StaticMesh'/Engine/EditorMeshes/MatineeCam_SM.MatineeCam_SM'"));
	if (camera.Object)
		camera_mesh->SetStaticMesh(camera.Object);

	camera_mesh->SetupAttachment(scene_camera);

	camera_path = "/dev/video0";
	plate_opacity = 0.5;
}

void ATrackingCamera::InitCamera()
{
	finished_init = false;
	setNumThreads(16); // speed

	cv_cap.open(TCHAR_TO_UTF8(*camera_path));

	if (cv_cap.isOpened())
	{
		if (!cv_cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G')))
			UE_LOG(LogTemp, Warning, TEXT("Could not set MJPG format"));

		if (!cv_cap.set(CAP_PROP_CONVERT_RGB, 0)) // UE OpenCV plugin broken, can't decompress jpeg, must do manually
			UE_LOG(LogTemp, Warning, TEXT("Could not enable raw output format"));

		if (!cv_cap.set(CAP_PROP_FRAME_WIDTH, resolution.X) || !cv_cap.set(
			CAP_PROP_FRAME_HEIGHT, resolution.Y))
			UE_LOG(LogTemp, Warning, TEXT("Could not set size"));

		if (!cv_cap.set(CAP_PROP_FPS, 60))
			UE_LOG(LogTemp, Warning, TEXT("Could not set fps"));

		if (!cv_cap.set(CAP_PROP_AUTOFOCUS, 0))
			UE_LOG(LogTemp, Warning, TEXT("Could not set autofocus"));

		if (!cv_cap.set(CAP_PROP_FOCUS, 0))
			UE_LOG(LogTemp, Warning, TEXT("Could not set focus"));

		if (!cv_cap.set(CAP_PROP_AUTO_EXPOSURE, 1)) // this means no auto exposure, ¯\_(ツ)_/¯
			UE_LOG(LogTemp, Warning, TEXT("Could not turn off auto exposure"));

		if (!cv_cap.set(CAP_PROP_EXPOSURE, exposure))
			UE_LOG(LogTemp, Warning, TEXT("Could not set exposure to %f"), exposure)


		cv_size = Size(cv_cap.get(CAP_PROP_FRAME_WIDTH), cv_cap.get(CAP_PROP_FRAME_HEIGHT));

		UE_LOG(LogTemp, Warning, TEXT("Opened camera at %s with %dx%d at %d fps"), *camera_path,
		       int(cv_cap.get(CAP_PROP_FRAME_WIDTH)), int(cv_cap.get(CAP_PROP_FRAME_HEIGHT)),
		       int(cv_cap.get(CAP_PROP_FPS)));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open camera at path: %s"), *camera_path);
		return;
	}

	ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);

	initUndistortRectifyMap(K(), p(), {}, {}, cv_size, CV_32FC1, cv_undistort_map1,
	                        cv_undistort_map2);
	
	cv_bg_subtractor = createBackgroundSubtractorMOG2();

	SimpleBlobDetector::Params cv_blob_params;
	memset(&cv_blob_params, 0, sizeof(SimpleBlobDetector::Params)); // TODO: reveal options in UE Editor

	cv_blob_params.minThreshold = 254;
	cv_blob_params.maxThreshold = 255;
	cv_blob_params.thresholdStep = 1;
	cv_blob_params.minRepeatability = 1;

	cv_blob_params.filterByConvexity = true;
	cv_blob_params.minConvexity = 0.6;
	cv_blob_params.maxConvexity = 1;

	cv_blob_params.filterByArea = true;
	cv_blob_params.minArea = 1000;
	cv_blob_params.maxArea = 50000;


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

	finished_init = true;
}

Mat ATrackingCamera::K() const
{
	return (cv::Mat_<double>(3, 3) <<
		focal_length.X, 0, cv_size.width / 2,
		0, focal_length.Y, cv_size.height / 2,
		0, 0, 1);
}

Mat ATrackingCamera::p() const
{
	return (cv::Mat_<double>(5, 1) << k_twins.X, k_twins.Y, p_twins.X, p_twins.Y, 0);
}


double ATrackingCamera::SyncFrame()
{
	if (!cv_cap.isOpened() && finished_init)
		return 0;
	
	cv_cap.grab();
	double time_captured = cv_cap.get(CAP_PROP_POS_MSEC);

	if (debug_output)
		UE_LOG(LogTemp, Display,TEXT("Camera %s grabbed frame at %f ms"), *camera_path, time_captured);

	return time_captured;
}

void ATrackingCamera::GetFrame()
{
	if (!cv_cap.isOpened() && finished_init)
		return;
		
	Mat cv_frame_raw, cv_frame_distorted;
	cv_cap.retrieve(cv_frame_raw);


	auto time_start = std::chrono::high_resolution_clock::now().time_since_epoch();
	
	uint8* UncompressedData = nullptr;
	TEnumAsByte<Decompressor> decompressor_used = decompressor;
	if (decompressor_used == STB)
	{
		// uncompress jpeg frame with stb single header library -> OpenCV plugin for UE's uncompression is broken for some reason :(
		// this is also the reason why getting raw data from webcam instead of uncompressed -> setting CAP_PROP_CONVERT_RGB to false
		int Width, Height, NumComponents;
		UncompressedData = stbi_load_from_memory(cv_frame_raw.data,
		                                         cv_frame_raw.size().area() * cv_frame_raw.elemSize(),
		                                         &Width, &Height, &NumComponents, STBI_rgb);
		if (Width != cv_size.width || Height != cv_size.height || NumComponents != 3)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("The frame wasn't decompressed properly (dimensions or number of components is incorrect)"));
			return;
		}
		cv_frame_distorted = Mat(cv_size, CV_8UC3, UncompressedData);
	}
	else if (decompressor_used == UJPEG)
	{
		uJPEG uj; // alternative decompressor, seems slower though...
		uj.setChromaMode(UJ_CHROMA_MODE_FAST);
		uj.decode(cv_frame_raw.data, cv_frame_raw.size().area() * cv_frame_raw.elemSize());

		if (uj.bad())
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not decode JPEG"))
			return;
		}

		if (uj.getWidth() != cv_size.width || uj.getHeight() != cv_size.height || uj.getImageSize() / uj.getWidth() / uj
			.getHeight() != 3)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("The frame wasn't decompressed properly (dimensions or number of components is incorrect)"));
			return;
		}
		cv_frame_distorted = Mat(cv_size, CV_8UC3, UncompressedData = (uint8*)uj.getImage());
	}
	else
	{
		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(cv_frame_raw.data,
		                                                          cv_frame_raw.size().area() * cv_frame_raw.elemSize()))
		{
			TArray<uint8> UncompressedBGRA;
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
			{
				cv_frame = Mat(cv_size, CV_8UC3);
				for (int i = 0; i < UncompressedBGRA.Num() / 4; i++) // copy the data
				{
					cv_frame_distorted.data[i * 3] = UncompressedBGRA[i * 4];
					cv_frame_distorted.data[i * 3 + 1] = UncompressedBGRA[i * 4 + 1];
					cv_frame_distorted.data[i * 3 + 2] = UncompressedBGRA[i * 4 + 2];
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Could not decode JPEG"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not set JPEG data or ImageWrapper is not valid"));
		}
	}

	auto time_end_uncompress = std::chrono::high_resolution_clock::now().time_since_epoch();

	if (debug_output)
		UE_LOG(LogTemp, Display, TEXT("Took camera %s %f ms to decompress frame at %d x %d"), *camera_path, (time_end_uncompress - time_start).count() / 1e6, cv_size.width, cv_size.height);
	
	remap(cv_frame_distorted, cv_frame, cv_undistort_map1, cv_undistort_map2, INTER_LINEAR);

	free(UncompressedData); // in case stb or ujpeg was used free the memory allocated by the library

	if (cv_frame.empty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Frame is empty after decompression"))
		return;
	}
	
}

Point2d ATrackingCamera::FindBall()
{
	if (!cv_cap.isOpened() && finished_init)
		return {};

	if (cv_frame.empty())
	{
		UE_LOG(LogTemp, Warning, TEXT("cv_frame is empty, cannot find ball"));
		return {};
	}

	auto time_before = chrono::high_resolution_clock::now();
	
	Mat cv_frame_scaled;
	float factor_used = processing_resolution_factor;
	resize(cv_frame, cv_frame_scaled, Size(), factor_used, factor_used, INTER_AREA);


	Mat cv_frame_HSV, cv_color_threshold, cv_bg_threshold, cv_threshold;
	cvtColor(cv_frame_scaled, cv_frame_HSV, COLOR_RGB2HSV);

	inRange(cv_frame_HSV, Scalar(low_H, low_S, low_V), Scalar(high_H, high_S, high_V), cv_color_threshold);

	if (learning_rate != 1) // used to "deactivate" the background subtraction
	{
		cv_bg_subtractor->apply(cv_frame_scaled, cv_bg_threshold, learning_rate);
		bitwise_and(cv_color_threshold, cv_bg_threshold, cv_threshold);
	}
	else
	{
		swap(cv_threshold, cv_color_threshold);
	}

	if (cv_threshold.empty())
	{
		UE_LOG(LogTemp, Warning, TEXT("threshold frame is empty!"));
		return {};
	}

	Point2f det = {-1, -1};

	Mat cv_debug_frame_temp;
	
	if (display_threshold_frame)
	{
		resize(cv_threshold, cv_debug_frame_temp, {}, 1 / factor_used, 1 / factor_used);
		cvtColor(cv_debug_frame_temp, cv_debug_frame_temp, COLOR_GRAY2RGB);
	} else
	{
		cv_debug_frame_temp = cv_frame;
	}

	if (detection_type == BlobDetector)
	{
		vector<KeyPoint> points;
		cv_blob_detector->detect(cv_threshold, points);


		sort(points.begin(), points.end(), [](const KeyPoint& a, const KeyPoint& b)
		{
			return a.size > b.size;
		});

		if (points.size())
		{
			det = points[0].pt / factor_used;

			const int radius = 50;
			circle(cv_debug_frame_temp, det, radius, Scalar(255, 0, 0), 3);
			line(cv_debug_frame_temp, det - Point2f(radius, 0), det + Point2f(radius, 0),
			     Scalar(255, 0, 0), 3);
			line(cv_debug_frame_temp, det - Point2f(0, radius), det + Point2f(0, radius),
			     Scalar(255, 0, 0), 3);
		}
	}
	else if (detection_type == ContourFilter)
	{
		// Find contours in the image
		vector<vector<Point>> contours;
		findContours(cv_threshold, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		int best_contour = -1;
		double area = -1;
		for (int i = 0; i < contours.size(); i++)
		{
			double c_area = contourArea(contours[i]);
			if (c_area > area)
			{
				area = c_area;
				best_contour = i;
			}
		}

		if (best_contour != -1 && area > min_blob_size * factor_used * factor_used) // at least 20x20 pixels seems reasonable
		{
			vector<vector<Point>> contours_to_draw = {{}};

			for (Point p : contours[best_contour])
				contours_to_draw[0].push_back(p / factor_used);

			drawContours(cv_debug_frame_temp, contours_to_draw, -1, cv::Scalar(0, 0, 255), 2);
			RotatedRect bounding_box = minAreaRect(contours[best_contour]);
			Point2f points[4];
			bounding_box.points(points);
			for (int i = 0; i < 4; i++)
				line(cv_debug_frame_temp, points[i] / factor_used, points[(i + 1) % 4] / factor_used, Scalar(255, 0, 0), 3);

			det = bounding_box.center / factor_used;
		}
	}

	if (det.x != -1 && det.y != -1)
	{
		ball_steps_skipped = 0;
		ball_path.push_back(det);

		if (debug_output)
			UE_LOG(LogTemp, Warning, TEXT("Camera %s detected ball at: %f %f!"), *camera_path, det.x, det.y);
	}
	else if (ball_steps_skipped++ == 5)
		ball_path.clear();

	for (int i = 0; i < int(ball_path.size()) - 1; i++)
	{
		line(cv_debug_frame_temp, ball_path[i], ball_path[i + 1], Scalar(0, 255, 0), 3);
	}

	auto time_after = chrono::high_resolution_clock::now();

	if (debug_output)
		UE_LOG(LogTemp, Display, TEXT("Took camera %s %f ms to find ball"), *camera_path, (time_after - time_before).count() / 1e6);

	cvtColor(cv_debug_frame_temp, cv_debug_frame, COLOR_RGB2RGBA);
	
	return ball = det;
}

void ATrackingCamera::RecalculateAverageTransform()
{
	FTransform average = FTransform::Identity;

	for (int i = 0; i < april_transforms.size(); i++)
	{
		average.Blend(average, april_transforms[i], 1 / double(i + 1));
	}

	average_april_transform = average;
}

void ATrackingCamera::DrawDetectedTags()
{
	for (auto det : last_tags)
	{
		line(cv_debug_frame, Point(det.p[0][0], det.p[0][1]),
			 Point(det.p[1][0], det.p[1][1]),
			 Scalar(0xff, 0, 0, 0xff), 2);
		line(cv_debug_frame, Point(det.p[0][0], det.p[0][1]),
			 Point(det.p[3][0], det.p[3][1]),
			 Scalar(0, 0xff, 0, 0xff), 2);
		line(cv_debug_frame, Point(det.p[1][0], det.p[1][1]),
			 Point(det.p[2][0], det.p[2][1]),
			 Scalar(0, 0, 0xff, 0xff), 2);
		line(cv_debug_frame, Point(det.p[2][0], det.p[2][1]),
			 Point(det.p[3][0], det.p[3][1]),
			 Scalar(0, 0, 0xff, 0xff), 2);

		string text = to_string(det.id);
		int fontface = FONT_HERSHEY_SCRIPT_SIMPLEX;
		double fontscale = 1.0;
		int baseline;
		Size textsize = getTextSize(text, fontface, fontscale, 2,
									&baseline);
		putText(cv_debug_frame, text, Point(det.c[0] - textsize.width / 2, det.c[1] + textsize.height / 2),
				fontface, fontscale, Scalar(0, 0x99, 0xff, 0xff), 2);
	}
}

FTransform ATrackingCamera::LocalizeCamera()
{
	if (!cv_cap.isOpened() && finished_init)
		return FTransform::Identity;

	if (cv_frame.empty())
	{
		UE_LOG(LogTemp, Warning, TEXT("cv_frame is empty, cannot localize camera"));
		return FTransform::Identity;
	}

	auto time_before = chrono::high_resolution_clock::now();
	
	Mat cv_frame_gray;
	cvtColor(cv_frame, cv_frame_gray, COLOR_RGB2GRAY);

	// Make an image_u8_t header for the Mat data
	image_u8_t im = {
		.width = cv_frame_gray.cols,
		.height = cv_frame_gray.rows,
		.stride = cv_frame_gray.cols,
		.buf = cv_frame_gray.data
	};

	if (cv_frame_gray.empty())
	{
		UE_LOG(LogTemp, Warning, TEXT("cv_frame is empty, cannot localize camera"));
		return FTransform::Identity;
	}

	zarray_t* detections = apriltag_detector_detect(at_td, &im);

	FTransform average_transform = FTransform::Identity;
	float total_transformations = 0;

	last_tags.clear();
	
	// Draw detection outlines
	for (int i = 0; i < zarray_size(detections); i++)
	{
		std::map<string, int> family_names = {
			{"tag16h5", 0}, {"tag25h9", 1}, {"tag36h11", 2}, {"tagCircle21h7", 3}, {"tagCircle49h12", 4},
			{"tagCustom48h12", 5}, {"tagStandard41h12", 6}, {"tagStandard52h13", 7}
		};

		apriltag_detection_t* det;
		zarray_get(detections, i, &det);

		last_tags.push_back(*det);
		
		FString name = det->family->name;
		
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
			info.tagsize = det_tag->tag_size * 100;
			info.fx = focal_length.X;
			info.fy = focal_length.Y;
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
			
			FMatrix tag_transformation = det_tag->mesh->GetComponentTransform().ToMatrixNoScale();
			FMatrix world_transformation = FQuat::MakeFromEuler(FVector(0, -90, -90)).ToMatrix() *
				relative_transformation.Inverse() * tag_transformation;

			average_transform.Blend(average_transform, FTransform(world_transformation),
			                        1 / (total_transformations + 1));
			total_transformations++;
		}
	}

	apriltag_detections_destroy(detections);

	auto time_after = chrono::high_resolution_clock::now();

	if (debug_output)
		UE_LOG(LogTemp, Display, TEXT("Took camera %s %f ms to find apriltags"), *camera_path, (time_after - time_before).count() / 1e6);
	
	return average_transform;
}

void ATrackingCamera::ReleaseCamera()
{
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
}

void ATrackingCamera::UpdateDebugTexture()
{
	if (!cv_cap.isOpened() && finished_init)
		return;
	
	if (!camera_texture_2d)
	{
		UE_LOG(LogTemp, Error, TEXT("camera texture pointer is null"));
		return;
	}

	if (cv_debug_frame.size().area() > 0)
	{
		void* texture_data = camera_texture_2d->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		memcpy(texture_data, cv_debug_frame.data, cv_debug_frame.elemSize() * cv_debug_frame.size().area());
		camera_texture_2d->GetPlatformData()->Mips[0].BulkData.Unlock();
		camera_texture_2d->UpdateResource();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("cv_debug_frame Mat is empty, could not update"));
	}
}

void ATrackingCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
}

// Called when the game starts or when spawned
void ATrackingCamera::BeginPlay()
{
	april_transforms.push_back(GetActorTransform());
	InitCamera();

	if (!cv_cap.isOpened())
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open camera"));
	}
	else
	{
		camera_texture_2d = UTexture2D::CreateTransient(cv_size.width, cv_size.height, PF_R8G8B8A8);
#if WITH_EDITORONLY_DATA
		camera_texture_2d->MipGenSettings = TMGS_NoMipmaps;
#endif
	}

	auto plate_config = image_plate->GetPlate();
	{
		if (plate_config.Material)
			plate_config.DynamicMaterial = UMaterialInstanceDynamic::Create(plate_config.Material, this);

		if (plate_config.DynamicMaterial)
			plate_config.DynamicMaterial->SetScalarParameterValue(FName("Opacity"), plate_opacity);

		plate_config.RenderTexture = camera_texture_2d;
	}
	image_plate->SetImagePlate(plate_config);

	InitCamera();
	
	Super::BeginPlay();
}

#if WITH_EDITOR
void ATrackingCamera::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{

	auto plate_config = image_plate->GetPlate();
	{
		if (plate_config.DynamicMaterial)
			plate_config.DynamicMaterial->SetScalarParameterValue(FName("Opacity"), plate_opacity);
		else
			UE_LOG(LogTemp, Warning, TEXT("Error setting material parameter: TrackingCamera"));
	}
	image_plate->SetImagePlate(plate_config);

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// Called every frame
void ATrackingCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateDebugTexture();

	if (ball != Point2d{-1, -1})
	{
		vector ball_point = {ball};
		vector<Point2d> output_points;
		undistortPoints(ball_point, output_points, K(), {});
		FVector homo_ball = {1, output_points[0].x, -output_points[0].y};
		
	
		 
		FVector origin = GetActorTransform().TransformPosition({0, 0, 0});
		FVector dir = GetActorTransform().TransformVector(homo_ball / homo_ball.Length());

		DrawDebugLine(GetWorld(), origin, origin + dir * 1000, FColor::Red, false, -1, 1, 3);
	}

	SetActorRelativeTransform(average_april_transform);
	
	camera_mesh->SetVisibility(!IsPlayerControlled());
	image_plate->SetVisibility(IsPlayerControlled() && cv_cap.isOpened());
}

void ATrackingCamera::BeginDestroy()
{
	ReleaseCamera();
	Super::BeginDestroy();
}

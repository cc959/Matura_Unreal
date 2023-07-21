// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <map>
#include <queue>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "ImagePlateComponent.h"
#include "MyUserWidget.h"
#include "IntVectorTypes.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "UObject/ObjectPtr.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"


#include "Tag.h"

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

#include "apriltags/apriltag.h"
#include "apriltags/tag16h5.h"
#include "apriltags/tag25h9.h"
#include "apriltags/tag36h11.h"
#include "apriltags/tagCircle21h7.h"
#include "apriltags/tagCircle49h12.h"
#include "apriltags/tagCustom48h12.h"
#include "apriltags/tagStandard41h12.h"
#include "apriltags/tagStandard52h13.h"
#include "apriltags/apriltag_pose.h"


#include "TrackingCamera.generated.h"

using namespace cv;
using namespace UE::Geometry;

UENUM()
enum DetectionType
{
	BlobDetector = 0 UMETA(DisplayName = "Blob Detector"),
	ContourFilter = 1 UMETA(DisplayName = "Contour Filter"),
};

UENUM()
enum Decompressor
{
	STB = 0 UMETA(DisplayName = "stb-image"),
	Turbo = 1 UMETA(DisplayName = "Turbo-JPEG"),
};

UENUM()
enum DebugFrameType
{
	None = 0 UMETA(DisplayName = "None"),
	Threshold = 1 UMETA(DisplayName = "Threshold"),
	HueOnly = 2 UMETA(DisplayName = "Hue Only"),
	SatOnly = 3 UMETA(DisplayName = "Saturation Only"),
	ValOnly = 4 UMETA(DisplayName = "Value Only"),
};

UCLASS()
class MATURA_UNREAL_API ATrackingCamera : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATrackingCamera();

	void InitCamera();
	void CreateTagDetector();

	double SyncFrame();
	__attribute__((target("mmx,sse2,avx2"))) void GetFrame();
	Point2d FindBall();
	double UpdateTransform(FTransform update);
	void DrawDetectedTags();
	std::pair<UE::Math::TTransform<double>, std::map<ATag*, UE::Math::TMatrix<double>>> UpdateTags(Mat frame);
	void ReleaseTagDetector();
	void FindTags();

	void ReleaseCamera();

	void UpdateDebugTexture();
	
	Size cv_size;

	Mat K() const;
	Mat p() const;
	Mutex destroy_lock;
	bool loaded = false;
	bool in_use = false;

	Point2d ball = {-1, -1};

	FTransform camera_transform;
	std::deque<FTransform> april_transforms;
	int64_t next_update_time = 0;

	Mat cv_frame;

	double last_frame_time;

	// the camera manager want to update tags, can't as is in other thread
	bool must_update_tags = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void RecalculateAverageTransform();

	VideoCapture cv_cap;
	
	Mat cv_debug_frame;
	Ptr<BackgroundSubtractor> cv_bg_subtractor;
	Ptr<SimpleBlobDetector> cv_blob_detector;
	apriltag_detector* at_td = nullptr;
	TArray<apriltag_family_t*> created_families;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper;

	Mat cv_undistort_map1, cv_undistort_map2;
	
	std::vector<Point2f> ball_path;
	int ball_steps_skipped = 0;

	Mutex last_tags_mut;
	std::vector<apriltag_detection_t> last_tags;


	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void BeginDestroy() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif
	virtual void SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) override;

	
	UPROPERTY(EditAnywhere)
	UCameraComponent *scene_camera;

	UPROPERTY(EditAnywhere)
	UImagePlateComponent *image_plate;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *camera_mesh;

	UPROPERTY(EditAnywhere)
	bool debug_output = false;

	UPROPERTY(EditAnywhere)
	bool update_texture = true;
	
	UPROPERTY(VisibleAnywhere, Category = WebCam)
	UTexture2D *camera_texture_2d;
	
	UPROPERTY(EditAnywhere, Category = WebCam)
	FString camera_path;

	UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", UIMax = "1.0"))
	float plate_opacity;

	UPROPERTY(EditAnywhere, Category=AprilTag)
	bool autodetect_tags;
	
	UPROPERTY(EditAnywhere, Category = AprilTag, meta=(EditCondition="!autodetect_tags", EditConditionHides))
	TArray<ATag *> april_tags;

	UPROPERTY(EditAnywhere, Category = AprilTag, DisplayName="Minimum Update Rate (s)")
	double update_rate = 1; // 100ms
	
	UPROPERTY(EditAnywhere, Category = CameraParams)
	FVector2D resolution;

	UPROPERTY(EditAnywhere, Category = CameraParams, meta = (UIMin = "0.1", UIMax = "1.0"))
	float processing_resolution_factor = 0.5;
	
	UPROPERTY(EditAnywhere, Category = CameraParams, meta = (UIMin = "0.0", UIMax = "1500.0"))
	float exposure = 750;
	
	UPROPERTY(EditAnywhere, Category = CameraParams)
	FVector2D focal_length;
	
	UPROPERTY(EditAnywhere, Category = CameraParams)
	FVector2D k_twins;

	UPROPERTY(EditAnywhere, Category = CameraParams)
	FVector2D p_twins;

	
	UPROPERTY(EditAnywhere, Category = BlobParams, meta = (UIMin = "0.0", UIMax = "180.0"))
	int low_H = 0;
	UPROPERTY(EditAnywhere, Category = BlobParams, meta = (UIMin = "0.0", UIMax = "255.0"))
	int low_S = 0;
	UPROPERTY(EditAnywhere, Category = BlobParams, meta = (UIMin = "0.0", UIMax = "255.0"))
	int low_V = 0;
	UPROPERTY(EditAnywhere, Category = BlobParams, meta = (UIMin = "0.0", UIMax = "180.0"))
	int high_H = 180;
	UPROPERTY(EditAnywhere, Category = BlobParams, meta = (UIMin = "0.0", UIMax = "255.0"))
	int high_S = 255;
	UPROPERTY(EditAnywhere, Category = BlobParams, meta = (UIMin = "0.0", UIMax = "255.0"))
	int high_V = 255;

	UPROPERTY(EditAnywhere, Category = BlobParams, meta = (UIMin = "-1.0", UIMax = "1.0"))
	float learning_rate = -1;

	UPROPERTY(EditAnywhere, Category = BlobParams, DisplayName = "Minimum Blob Size (px)")
	int min_blob_size = 300;

	UPROPERTY(EditAnywhere, Category = BlobParams)
	TEnumAsByte<DebugFrameType> debug_frame_type = None;

	UPROPERTY(EditAnywhere, Category = BlobParams, meta = (EditCondition = "debug_frame_type != 0", EditConditionHides))
	bool apply_threshold_to_debug_frame = false;

	UPROPERTY(EditAnywhere, Category = BlobParams)
	bool draw_debug_overlay = true;
	
	UPROPERTY(EditAnywhere, Category = BlobParams)
	TEnumAsByte<DetectionType> detection_type = DetectionType::BlobDetector;
	
	UPROPERTY(EditAnywhere, Category = BlobParams)
	TEnumAsByte<Decompressor> decompressor = Decompressor::STB;
	
};

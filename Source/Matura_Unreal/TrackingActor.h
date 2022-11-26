// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Tag.h"

#include "PreOpenCVHeaders.h"
	#include <opencv2/core.hpp>
	#include <opencv2/imgproc.hpp>
	#include <opencv2/videoio.hpp>
#include "PostOpenCVHeaders.h"

extern "C"
{
#include "apriltags/apriltag.h"
#include "apriltags/apriltag_pose.h"
#include "apriltags/tag36h11.h"
#include "apriltags/tag25h9.h"
#include "apriltags/tag16h5.h"
#include "apriltags/tagCircle21h7.h"
#include "apriltags/tagCircle49h12.h"
#include "apriltags/tagCustom48h12.h"
#include "apriltags/tagStandard41h12.h"
#include "apriltags/tagStandard52h13.h"
#include "apriltags/common/getopt.h"
}

#include "TrackingActor.generated.h"

using namespace cv;

UCLASS()
class MATURA_UNREAL_API ATrackingActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATrackingActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void BeginDestroy() override;

	VideoCapture cv_cap;
	Size cv_size;
	Mat cv_frame;
	Mat cv_frame_gray;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *mesh;

	UPROPERTY(VisibleAnywhere)
	UTexture2D *camera_texture_2d;

	FArr

	apriltag_detector *at_td;
};

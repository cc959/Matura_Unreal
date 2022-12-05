// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "ImagePlateComponent.h"
#include "MyUserWidget.h"

#include "Tag.h"

#include "CameraManager.h"


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

#include "TrackingCamera.generated.h"

using namespace cv;

UCLASS()
class MATURA_UNREAL_API ATrackingCamera : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATrackingCamera();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void BeginDestroy() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
	virtual void SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) override;

	
	UPROPERTY(EditAnywhere)
	UCameraComponent *scene_camera;

	UPROPERTY(EditAnywhere)
	UImagePlateComponent *image_plate;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *camera_mesh;

	UPROPERTY(VisibleAnywhere, Category = WebCam)
	UTexture2D *camera_texture_2d;

	UPROPERTY(EditAnywhere, Category = WebCam)
	int camera_id;

	UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", UIMax = "1.0"))
	float plate_opacity;

	UPROPERTY(EditAnywhere)
	TArray<ATag *> april_tags;

	CameraManager* camera_manager;
	
};

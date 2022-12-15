// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "ImagePlateComponent.h"
#include "MyUserWidget.h"

#include "Tag.h"

#include "CameraManager.h"

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

	UPROPERTY(EditAnywhere, Category = CameraParams, meta = (UIMin = "0.0", UIMax = "1500.0"))
	float exposure = 750;
	
	UPROPERTY(EditAnywhere, Category = CameraParams)
	FVector2D focal_length;

	
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

	UPROPERTY(EditAnywhere, Category = BlobParams)
	bool display_threshold_frame = false;
	
	
	class CameraManager* camera_manager;
	
};

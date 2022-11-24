// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PreOpenCVHeaders.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "PostOpenCVHeaders.h"

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
	Mat cv_mat;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *mesh;

	UPROPERTY(VisibleAnywhere)
	UTexture2D *camera_texture_2d;
};

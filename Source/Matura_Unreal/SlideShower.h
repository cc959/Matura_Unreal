// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <ImagePlateComponent.h>

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "SlideShower.generated.h"

/**
 * 
 */
UCLASS()
class MATURA_UNREAL_API ASlideShower : public ACameraActor
{
	GENERATED_BODY()

	UTexture2D* slide_texture;
	
public:

	ASlideShower();
	virtual void BeginPlay() override;
	void UpdatePlate();


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif	
	UPROPERTY(EditAnywhere)
	UImagePlateComponent *image_plate;

	UPROPERTY(EditAnywhere)
	int slide_index;
	
};

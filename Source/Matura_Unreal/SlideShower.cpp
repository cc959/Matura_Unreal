// Fill out your copyright notice in the Description page of Project Settings.


#include "SlideShower.h"

ASlideShower::ASlideShower()
{
	image_plate = CreateDefaultSubobject<UImagePlateComponent>(FName("Slide Plate"));
	image_plate->SetupAttachment(RootComponent);
	image_plate->SetRelativeLocation(FVector(11, 0, 0));

	// static ConstructorHelpers::FObjectFinder<UMaterialInterface> plate_material(
	// 	TEXT("/Script/Engine.Material'/Game/Unlit.Unlit'"));
	// if (plate_material.Object)
	// 	image_plate->SetMaterial(0, plate_material.Object);
}

void ASlideShower::BeginPlay()
{
	Super::BeginPlay();
	UpdatePlate();

	
}

void ASlideShower::UpdatePlate()
{

	std::string path = "/Game/Slides/Slide_" + std::to_string(slide_index) + ".Slide_"+std::to_string(slide_index);
	
	slide_texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), this, *FString(path.c_str())));
	
	auto plate_config = image_plate->GetPlate();
	{
		if (plate_config.Material)
			plate_config.DynamicMaterial = UMaterialInstanceDynamic::Create(plate_config.Material, this);

		plate_config.RenderTexture = slide_texture;
	}
	image_plate->SetImagePlate(plate_config);
}

#if WITH_EDITOR
void ASlideShower::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdatePlate();
}
#endif



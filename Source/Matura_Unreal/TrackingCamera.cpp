// Fill out your copyright notice in the Description page of Project Settings.

#include "TrackingCamera.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "UObject/ObjectPtr.h"

#include <string>
#include <functional>
#include <set>
#include <map>

using namespace std;

// Sets default values
ATrackingCamera::ATrackingCamera()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	scene_camera = CreateDefaultSubobject<UCameraComponent>(FName("Scene Camera"));
	scene_camera->SetAspectRatio(1);
	scene_camera->bCameraMeshHiddenInGame = false;
	scene_camera->bConstrainAspectRatio = true;
	scene_camera->SetCameraMesh(NULL);
	SetRootComponent(scene_camera);

	image_plate = CreateDefaultSubobject<UImagePlateComponent>(FName("Debug Image Plate"));
	image_plate->SetupAttachment(scene_camera);
	image_plate->SetRelativeLocation(FVector(11, 0, 0));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> plate_material(TEXT("/Script/Engine.Material'/Game/Unlit.Unlit'"));
	if (plate_material.Object)
		image_plate->SetMaterial(0, plate_material.Object);

	camera_mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Camera Mesh"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> camera(TEXT("/Script/Engine.StaticMesh'/Engine/EditorMeshes/MatineeCam_SM.MatineeCam_SM'"));
	if (camera.Object)
		camera_mesh->SetStaticMesh(camera.Object);

	camera_mesh->SetupAttachment(scene_camera);
	
	camera_id = 0;
	plate_opacity = 0.5;
}

void ATrackingCamera::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
}

// Called when the game starts or when spawned
void ATrackingCamera::BeginPlay()
{

	camera_manager = new CameraManager(april_tags, camera_id, this);

	if (!camera_manager->cv_cap.isOpened())
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open camera"));
	}
	else
	{
		camera_texture_2d = UTexture2D::CreateTransient(camera_manager->cv_size.width, camera_manager->cv_size.height, PF_B8G8R8A8);
		camera_texture_2d->MipGenSettings = TMGS_NoMipmaps;
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

	Super::BeginPlay();
}

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

// Called every frame
void ATrackingCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetActorRelativeTransform(camera_manager->world_transform);
	camera_manager->GetTexture(camera_texture_2d);
	
	camera_mesh->SetVisibility(!IsPlayerControlled());
	image_plate->SetVisibility(IsPlayerControlled() && camera_manager->cv_cap.isOpened());
}

void ATrackingCamera::BeginDestroy()
{
	delete camera_manager;
	Super::BeginDestroy();
}

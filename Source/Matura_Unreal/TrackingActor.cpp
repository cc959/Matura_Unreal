// Fill out your copyright notice in the Description page of Project Settings.

#include "TrackingActor.h"

// Sets default values
ATrackingActor::ATrackingActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static mesh"));
	SetRootComponent(mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Game/StarterContent/Shapes/Shape_Plane.Shape_Plane"));

	if (PlaneMesh.Object)
		mesh->SetStaticMesh(PlaneMesh.Object);
}

// Called when the game starts or when spawned
void ATrackingActor::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("Blub"));
	Super::BeginPlay();
	cv_cap.open(0, CAP_ANY);

	if (!cv_cap.isOpened())
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not open camera"));
	}
	else
	{
		cv_size = Size(cv_cap.get(CAP_PROP_FRAME_WIDTH), cv_cap.get(CAP_PROP_FRAME_HEIGHT));

		camera_texture_2d = UTexture2D::CreateTransient(cv_size.width, cv_size.height, PF_B8G8R8A8);
		camera_texture_2d->MipGenSettings = TMGS_NoMipmaps;

		UMaterialInstanceDynamic *material = UMaterialInstanceDynamic::Create(mesh->GetMaterial(0), this);
		material->SetTextureParameterValue(FName(TEXT("Texture")), camera_texture_2d);
		mesh->SetMaterial(0, material);
	}
}

// Called every frame
void ATrackingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (cv_cap.isOpened())
	{
		cv_cap >> cv_mat;
		cvtColor(cv_mat, cv_mat, COLOR_BGR2BGRA);

		void *texture_data = camera_texture_2d->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		memcpy(texture_data, cv_mat.data, cv_mat.elemSize() * cv_mat.size().area());
		camera_texture_2d->PlatformData->Mips[0].BulkData.Unlock();
		camera_texture_2d->UpdateResource();
	}
}

void ATrackingActor::BeginDestroy()
{
	Super::BeginDestroy();

	cv_cap.release();
}

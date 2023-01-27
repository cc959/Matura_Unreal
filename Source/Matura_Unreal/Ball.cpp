// Fill out your copyright notice in the Description page of Project Settings.


#include "Ball.h"

// Sets default values
ABall::ABall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ball_mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Ball Mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ball_finder(
			TEXT("/Engine/BasicShapes/Sphere"));
	if (ball_finder.Object)
		ball_mesh->SetStaticMesh(ball_finder.Object);
	SetRootComponent(ball_mesh);

	SetActorScale3D(FVector(display_size, display_size, display_size));
}

#if WITH_EDITOR
void ABall::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	SetActorScale3D(FVector(display_size, display_size, display_size));

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// Called when the game starts or when spawned
void ABall::BeginPlay()
{
	Super::BeginPlay();
	
	// for (ATrackingCamera* camera : tracking_cameras)
	// 	camera->InitCamera();
	
	manager = new CameraManager(this);
}

// Called every frame
void ABall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
 	if (!position.ContainsNaN())
		SetActorLocation(position);

	manager->DrawBallHistory();
}

void ABall::BeginDestroy()
{
	UE_LOG(LogTemp, Warning, TEXT("Ball is being destroyed"));

	if (manager)
		delete manager;

	UE_LOG(LogTemp, Warning, TEXT("Ball is done being destroyed"));

	
	// for (ATrackingCamera* camera : tracking_cameras)
	// 	camera->ReleaseCamera();

	Super::BeginDestroy();
}



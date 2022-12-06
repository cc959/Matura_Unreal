// Fill out your copyright notice in the Description page of Project Settings.


#include "PathPredictor.h"

// Sets default values
APathPredictor::APathPredictor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APathPredictor::BeginPlay()
{
	Super::BeginPlay();
	predictor.addPoint(0, FVector(0, 0, 0));
	predictor.addPoint(1, FVector(1, 1, 10 - 9.81/2));
	predictor.addPoint(2, FVector(2, 2, 10*2 - 9.81*4/2));
}

// Called every frame
void APathPredictor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	double step = (end_time - start_time) / double(num_steps);
	for (double t = start_time; t < end_time; t += step)
	{
		DrawDebugLine(GetWorld(), predictor(t) * 100, predictor(t+step) * 100, FColor::Green, false, 0, 1, 3);
	}
}


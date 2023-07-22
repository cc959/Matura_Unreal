// Fill out your copyright notice in the Description page of Project Settings.


#include "ParabolaTest.h"

// Sets default values
AParabolaTest::AParabolaTest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AParabolaTest::BeginPlay()
{
	Super::BeginPlay();

	if (WITH_EDITOR)
	{
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bStartWithTickEnabled = true;
	}
	
}

// Called every frame
void AParabolaTest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	std::vector<Position> positions;

	for (int i = 0; i < targets.Num(); i++)
	{
		if (targets[i])
			positions.push_back(Position{targets[i]->GetActorLocation(), double(i)+1});
	}

	if (positions.size() < 3)
		return;
	
	path = ParabPath::fromNPoints(positions);
	path.Draw(GetWorld(), FColor::Red, 10, 1, -1);

	DrawDebugSphere(GetWorld(), center, radius, 100, FColor::Blue, 0, -1);
	
	for (auto t : path.IntersectSphere(center, radius))
	{
		UE_LOG(LogTemp, Display, TEXT("%f"), t);
		DrawDebugSphere(GetWorld(), path(t), 20, 10, FColor::Green, 0, -1, 2, 3);
	}
}

bool AParabolaTest::ShouldTickIfViewportsOnly() const
{
	return true;
}



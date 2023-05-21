// Fill out your copyright notice in the Description page of Project Settings.


#include "WireSpline.h"

// Sets default values
AWireSpline::AWireSpline()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	spline_component = CreateDefaultSubobject<USplineComponent>("Spline");
	
	if (spline_component)
		SetRootComponent(spline_component);
}

// Called when the game starts or when spawned
void AWireSpline::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void AWireSpline::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWireSpline::OnConstruction(const FTransform& Transform)
{
	if (mesh)
	{
		for (int spline_count = 0; spline_count < spline_component->GetNumberOfSplinePoints() - 1; spline_count++)
		{
			USplineMeshComponent* spline_mesh_component = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass());

			spline_mesh_component->SetStaticMesh(mesh);
			spline_mesh_component->SetMobility(EComponentMobility::Movable);
			spline_mesh_component->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			spline_mesh_component->RegisterComponentWithWorld(GetWorld());
			spline_mesh_component->AttachToComponent(spline_component, FAttachmentTransformRules::KeepRelativeTransform);

			for (int i = 0; i < material_slots.Num(); i++)
				spline_mesh_component->SetMaterial(i, material_slots[i]);

			FVector start_point = spline_component->GetLocationAtSplinePoint(spline_count, ESplineCoordinateSpace::Local);
			FVector start_tangent = spline_component->GetTangentAtSplinePoint(spline_count, ESplineCoordinateSpace::Local);
			float start_roll = spline_component->GetRollAtSplinePoint(spline_count, ESplineCoordinateSpace::Local);

			FVector end_point = spline_component->GetLocationAtSplinePoint(spline_count + 1, ESplineCoordinateSpace::Local);
			FVector end_tangent = spline_component->GetTangentAtSplinePoint(spline_count + 1, ESplineCoordinateSpace::Local);
			float end_roll = spline_component->GetRollAtSplinePoint(spline_count + 1, ESplineCoordinateSpace::Local);
			
			spline_mesh_component->SetStartAndEnd(start_point, start_tangent, end_point, end_tangent, true);
			spline_mesh_component->SetStartRoll(start_roll / 180 * PI);
			spline_mesh_component->SetEndRoll(end_roll / 180 * PI);
		}
	}

	Super::OnConstruction(Transform);
}

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

void AWireSpline::ReconstructSpline()
{
	DestroyConstructedComponents();
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
			double start_roll = spline_component->GetRollAtSplinePoint(spline_count, ESplineCoordinateSpace::Local);
			FVector start_scale = spline_component->GetScaleAtSplinePoint(spline_count);

			FVector end_point = spline_component->GetLocationAtSplinePoint(spline_count + 1, ESplineCoordinateSpace::Local);
			FVector end_tangent = spline_component->GetTangentAtSplinePoint(spline_count + 1, ESplineCoordinateSpace::Local);
			double end_roll = spline_component->GetRollAtSplinePoint(spline_count + 1, ESplineCoordinateSpace::Local);
			FVector end_scale = spline_component->GetScaleAtSplinePoint(spline_count + 1);
			
			spline_mesh_component->SetStartAndEnd(start_point, start_tangent, end_point, end_tangent, false);
			spline_mesh_component->SetStartRoll(start_roll / 180 * PI, false);
			spline_mesh_component->SetEndRoll(end_roll / 180 * PI, false);
			spline_mesh_component->SetStartScale(FVector2D(start_scale.X, start_scale.Y), false);
			spline_mesh_component->SetEndScale(FVector2D(end_scale.X, end_scale.Y), false);

			spline_mesh_component->UpdateMesh();
		}
	}
}


// Called every frame
void AWireSpline::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (must_reconstruct)
	{
		ReconstructSpline();
		must_reconstruct = false;
	}
}

void AWireSpline::OnConstruction(const FTransform& Transform)
{
	ReconstructSpline();

	Super::OnConstruction(Transform);
}

bool AWireSpline::ShouldTickIfViewportsOnly() const
{
	return true;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "WireHandle.h"

#include "FlyCharacter.h"

// Sets default values
UWireHandle::UWireHandle()
{
	TransformUpdated.AddLambda([&](USceneComponent* Component, EUpdateTransformFlags flags, ETeleportType type){OnMove();});
}

// Called when the game starts or when spawned
void UWireHandle::BeginPlay()
{
	Super::BeginPlay();
	
}

void UWireHandle::OnMove()
{
	if (wire_spline && point_index < wire_spline->spline_component->GetNumberOfSplinePoints())
	{
		wire_spline->spline_component->SetLocationAtSplinePoint(point_index, GetComponentLocation(), ESplineCoordinateSpace::World, false);
		wire_spline->spline_component->SetRotationAtSplinePoint(point_index, GetComponentRotation(), ESplineCoordinateSpace::World, false);
		wire_spline->spline_component->SetTangentAtSplinePoint(point_index, GetForwardVector() * tangent_weight * 100, ESplineCoordinateSpace::World, false);
		
		wire_spline->must_reconstruct = true;
	}
}


#if WITH_EDITOR
void UWireHandle::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	SetVisibility(IsSelectedInEditor() || IsOwnerSelected());
	SetArrowLength(ArrowSize * tangent_weight * 100);
	
	OnMove();

}
#endif

bool UWireHandle::ShouldRenderSelected() const
{
	return Super::ShouldRenderSelected();
}





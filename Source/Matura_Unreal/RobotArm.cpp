// Fill out your copyright notice in the Description page of Project Settings.

#include "RobotArm.h"

// Sets default values
ARobotArm::ARobotArm()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	base_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh0"));
	lower_arm_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh1"));
	upper_arm_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh2"));

	SetRootComponent(base_component);

	lower_arm_component->SetupAttachment(base_component);
	upper_arm_component->SetupAttachment(lower_arm_component);
}

// Called when the game starts or when spawned
void ARobotArm::BeginPlay()
{
	base_component->SetMaterial(0, UMaterialInstanceDynamic::Create(base_component->GetMaterial(0), this));

	Super::BeginPlay();
}

void ARobotArm::UpdateRotations()
{
	auto base_rotation = base_component->GetRelativeRotation().Euler();
	auto lower_arm_rotation = lower_arm_component->GetRelativeRotation().Euler();
	auto upper_arm_rotation = upper_arm_component->GetRelativeRotation().Euler();

	base_rotation.Z = baseRotation;
	lower_arm_rotation.X = lowerArmRotation;
	upper_arm_rotation.X = upperArmRotation;

	base_component->SetRelativeRotation(FQuat::MakeFromEuler(base_rotation));
	lower_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(lower_arm_rotation));
	upper_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(upper_arm_rotation));
}

// Called every frame
void ARobotArm::Tick(float DeltaTime)
{
	UpdateRotations();

	float realtimeSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());

	UMaterialInstanceDynamic *material = (UMaterialInstanceDynamic *)base_component->GetMaterial(0); // UMaterialInstanceDynamic::Create(base_component->GetMaterial(0), this);

	FLinearColor HSV(realtimeSeconds * 10.f, 1, 1);

	material->SetVectorParameterValue(FName(TEXT("Param")), HSV.HSVToLinearRGB());

	base_component->SetMaterial(0, material);

	Super::Tick(DeltaTime);
}

#ifdef WITH_EDITOR
void ARobotArm::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	UpdateRotations();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
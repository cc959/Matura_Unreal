// Fill out your copyright notice in the Description page of Project Settings.

#include "Tag.h"

#include <string>
#include <sstream>
#include <iomanip>

// Sets default values
ATag::ATag()
{
	PrimaryActorTick.bCanEverTick = false;
	tag_size = 1;

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static mesh"));
	SetRootComponent(mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Game/StarterContent/Shapes/Shape_Plane.Shape_Plane"));

	if (PlaneMesh.Object)
		mesh->SetStaticMesh(PlaneMesh.Object);

	ConstructorHelpers::FObjectFinder<UMaterial> material(TEXT("/Game/apriltag-imgs/UnlitTag.UnlitTag"));
	if (material.Object)
		mesh->SetMaterial(0, material.Object);

	UpdateScale();
}

void ATag::Clicked(UPrimitiveComponent *Target, FKey ButtonPressed)
{
	UE_LOG(LogTemp, Warning, TEXT("Clicked!"));
}

void ATag::OnConstruction(const FTransform &transform)
{
	Super::OnConstruction(transform);
}

void ATag::UpdateScale()
{
	static const double scale_factor[] = {8./6., 9./7., 10./8., 9./5., 11./5., 10./6., 9./5., 10./6.};
	mesh->SetWorldScale3D(FVector(tag_size * scale_factor[tag_family]));
}

#if WITH_EDITOR
void ATag::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	UpdateTexture();

	UpdateScale();
	
	Super::PostEditChangeProperty(PropertyChangedEvent);

}
#endif

void ATag::UpdateTexture()
{
	static const int max_id[] = {29, 34, 127, 37, 127, 127, 127, 127};
	tag_id = FMath::Clamp(tag_id, 0, max_id[tag_family]);

	static const std::string family_names[] = {"tag16h5", "tag25h9", "tag36h11", "tagCircle21h7", "tagCircle49h12", "tagCustom48h12", "tagStandard41h12", "tagStandard52h13"};
	static const std::string tag_pre[] = {"tag16_05_", "tag25_09_", "tag36_11_", "tag21_07_", "tag49_12_", "tag48_12_", "tag41_12_",  "tag52_13_"};

	std::stringstream ss;
	ss << std::setw(5) << std::setfill('0') << tag_id;
	std::string tag_id_string = ss.str();

	std::string path = "/Game/apriltag-imgs/" + family_names[tag_family] + "/" + tag_pre[tag_family] + tag_id_string + "." + tag_pre[tag_family] + tag_id_string;

	tag_texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), this, *FString(path.c_str())));

	if (tag_texture)
	{
		UMaterialInstanceDynamic *dyn_material;
		UMaterialInterface *material = mesh->GetMaterial(0);

		if (material->IsA(UMaterialInstanceDynamic::StaticClass()))
		{
			dyn_material = (UMaterialInstanceDynamic *)material;
		}
		else
		{
			dyn_material = UMaterialInstanceDynamic::Create(material, this);
		}
		dyn_material->SetTextureParameterValue(FName(TEXT("Texture")), tag_texture);
		mesh->SetMaterial(0, dyn_material);
	}
}

// Called when the game starts or when spawned
void ATag::BeginPlay()
{
	mesh->OnClicked.AddDynamic(this, &ATag::Clicked);

	UpdateScale();
	
	Super::BeginPlay();
}

// Called every frame
void ATag::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

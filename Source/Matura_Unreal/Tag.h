// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"

#include "Tag.generated.h"

UENUM()
enum TagFamily
{
	tag16h5 = 0 UMETA(DisplayName = "16h5"),
	tag25h9 = 1 UMETA(DisplayName = "25h9"),
	tag36h11 = 2 UMETA(DisplayName = "36h11"),
	tagCircle21h7 = 3 UMETA(DisplayName = "Circle21h7"),
	tagCircle49h12 = 4 UMETA(DisplayName = "Circle49h12"),
	tagCurtom48h12 = 5 UMETA(DisplayName = "Curtom48h12"),
	tagStandard52h13 = 6 UMETA(DisplayName = "Standard52h13")
};

UCLASS()
class MATURA_UNREAL_API ATag : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATag();
	virtual void OnConstruction(const FTransform& transform) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void UpdateTexture();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;

	UFUNCTION()
	void Clicked(UPrimitiveComponent *Target, FKey ButtonPressed);

	UPROPERTY(EditAnywhere, Category = Tag)
	TEnumAsByte<TagFamily> tag_family;

	UPROPERTY(EditAnywhere, Category = Tag)
	int tag_id;

	UPROPERTY(EditAnywhere, Category = Tag)
	float tag_size;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *mesh;
	
	UPROPERTY(EditAnywhere)
	UTexture2D *tag_texture;
};

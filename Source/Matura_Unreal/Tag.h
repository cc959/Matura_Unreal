// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <deque>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Tag.generated.h"

UENUM()
enum TagFamily
{
	tag16h5 = 0 UMETA(DisplayName = "16h5"),
	tag25h9 = 1 UMETA(DisplayName = "25h9"),
	tag36h11 = 2 UMETA(DisplayName = "36h11"),
	tagCircle21h7 = 3 UMETA(DisplayName = "Circle21h7"),
	tagCircle49h12 = 4 UMETA(DisplayName = "Circle49h12"),
	tagCustom48h12 = 5 UMETA(DisplayName = "Custom48h12"),
	tagStandard41h12 = 6 UMETA(DisplayName = "Standard41h12"),
	tagStandard52h13 = 7 UMETA(DisplayName = "Standard52h13")
};

UENUM()
enum TagType
{
	Static = 0,
	Dynamic = 1,
};

UCLASS()
class MATURA_UNREAL_API ATag : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATag();
	void UpdateScale();

	FTransform tag_transform;
	std::deque<FTransform> april_transforms;
	std::mutex transform_lock;

	double UpdateTransform(FTransform update);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void RecalculateAverageTransform();
	void UpdateTexture();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif
	UFUNCTION()
	void Clicked(UPrimitiveComponent *Target, FKey ButtonPressed);

	UPROPERTY(EditAnywhere, Category = Tag)
	TEnumAsByte<TagFamily> tag_family;

	UPROPERTY(EditAnywhere, Category = Tag)
	int tag_id;

	UPROPERTY(EditAnywhere, Category = Tag)
	TEnumAsByte<TagType> tag_type = TagType::Static;

	UPROPERTY(EditAnywhere, Category = Tag, DisplayName="Minimum Update Rate (s)", meta=(EditCondition="tag_type == TagType::Dynamic", EditConditionHides))
	double update_rate = 1;
	
	UPROPERTY(EditAnywhere, Category = Tag)
	double tag_size;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *mesh;
	
	UPROPERTY(EditAnywhere)
	UTexture2D *tag_texture;
	
};

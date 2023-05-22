// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FlyCharacter.h"
#include "WireSpline.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "WireHandle.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MATURA_UNREAL_API UWireHandle : public UArrowComponent
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	UWireHandle();
	
	void OnMove();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	virtual bool ShouldRenderSelected() const override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float tangent_weight = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AWireSpline* wire_spline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int point_index;
	
};

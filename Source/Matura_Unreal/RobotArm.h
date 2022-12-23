// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "RobotArm.generated.h"

UCLASS()
class MATURA_UNREAL_API ARobotArm : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARobotArm();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void SetupSerial();

	void UpdateRotations();
	void SendRotations();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool visual_only = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SerialInfo, meta = (EditCondition = "visual_only == false", EditConditionHides))
	FString port;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SerialInfo, meta = (EditCondition = "visual_only == false", EditConditionHides))
	int serial_port;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *base_component;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *lower_arm_component;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *upper_arm_component;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *wrist_component;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "0.0", UIMax = "180.0"))
	float base_rotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "0.0", UIMax = "127.0"))
	float lower_arm_rotation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "0.0", UIMax = "180.0"))
	float upper_arm_rotation;
    	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "0.0", UIMax = "180.0"))
	float wrist_rotation;
};

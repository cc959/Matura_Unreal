// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ball.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "RobotArm.generated.h"

UENUM()
enum UpdateType
{
	User = 0,
	Animation = 1,
	IK = 2,
	Ball = 3,
};

UCLASS()
class MATURA_UNREAL_API ARobotArm : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARobotArm();

protected:
	static constexpr double min_rotations[5] = {-242, -90, -217.5, -90, 0};
	static constexpr double max_rotations[5] = {0, 90, 37.5, 90, 180};

	static constexpr double min_servo[5] = {180, 130, 0, 180, 0};
	static constexpr double max_servo[5] = {4, 3, 180, 0, 180};

	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void SetupSerial();
	int NumOverlaps();

	void UpdateRotations();
	void SendRotations();

	bool UpdateIK(FVector target);
	void TrackBall();

	void SerialLoop();
	FVector ArmOrigin() const;
	bool serial_loop_running = true;
	TFuture<void> serial_thread;
	
	std::tuple<double, double, double, double, double> last_rotations;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;

	virtual bool ShouldTickIfViewportsOnly() const override;

#endif

	virtual void BeginDestroy() override;
	
	UPROPERTY(EditAnywhere)
	bool visual_only = true;

	UPROPERTY(EditAnywhere);
	bool check_collision = true;
	
	UPROPERTY(EditAnywhere)
	bool update_rotations = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "check_collision == true", EditConditionHides))
	TArray<AStaticMeshActor*> colliders;
	
	UPROPERTY(EditAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	FString port;

	UPROPERTY(VisibleAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	int serial_port = -1;

	UPROPERTY(EditAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	bool debug_serial = false;
	
	UPROPERTY(EditAnywhere)
	AStaticMeshActor *base_component;
	
	UPROPERTY(EditAnywhere)
	AStaticMeshActor *lower_arm_component;

	UPROPERTY(EditAnywhere)
	AStaticMeshActor *upper_arm_component;

	UPROPERTY(EditAnywhere)
	AStaticMeshActor *wrist_component;

	UPROPERTY(EditAnywhere)
	AStaticMeshActor *hand_component;
	
	// UPROPERTY(EditAnywhere)
	// AStaticMeshActor *hand_component;
	
	UPROPERTY(EditAnywhere, Category = Motors)
	TEnumAsByte<UpdateType> update_type;
	
	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="IK Target", meta=(EditCondition = "update_type == UpdateType::IK", EditConditionHides))
	AActor* ik_target;

	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	ABall* ball;

	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	bool draw_debug = false;

	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Lower arm length (m)", meta=(EditCondition = "update_type == UpdateType::IK || update_type == UpdateType::Ball", EditConditionHides))
	double lower_arm_length = 0.35;
	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Upper arm length (m)", meta=(EditCondition = "update_type == UpdateType::IK || update_type == UpdateType::Ball", EditConditionHides))
	double upper_arm_length = 0.265;

	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Hand length (m)", meta=(EditCondition = "update_type == UpdateType::IK || update_type == UpdateType::Ball", EditConditionHides))
	double hand_length = 0.08;

	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Arm range (m)", meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	double arm_range = 0.6;

	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	FVector impact;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-242.0", UIMax = "0.0", EditCondition = "update_type == UpdateType::User"))
	double base_rotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-90.0", UIMax = "90.0", EditCondition = "update_type == UpdateType::User"))
	double lower_arm_rotation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-217.5", UIMax = "37.5", EditCondition = "update_type == UpdateType::User"))
	double upper_arm_rotation;
    	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-90.0", UIMax = "90.0", EditCondition = "update_type == UpdateType::User"))
	double wrist_rotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "0.0", UIMax = "180.0", EditCondition = "update_type == UpdateType::User"))
	double hand_rotation;

};

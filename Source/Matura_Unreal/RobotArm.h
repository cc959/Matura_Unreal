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
	LinearPath = 4,
};

// the relevant ones all managed about 180 deg/s
const double motor_speed = 180;

UCLASS()
class MATURA_UNREAL_API ARobotArm : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARobotArm();

protected:
	static constexpr double min_rotations[5] = {-244, -90, -225, -75, 0};
	static constexpr double max_rotations[5] = {6, 90, 33, 95, 180};

	static constexpr double min_servo[5] = {180, 130, 0, 180, 0};
	static constexpr double max_servo[5] = {0, 3, 180, 0, 180};

	struct Position
	{
		double base_rotation;
		double lower_arm_rotation;
		double upper_arm_rotation;
		double hand_rotation;
		double wrist_rotation;

		Position(double v = nan(""))
		{
			base_rotation = lower_arm_rotation = upper_arm_rotation = hand_rotation = wrist_rotation = v;
		}

		double diff(Position other)
		{
			return max({
				abs(base_rotation - other.base_rotation) / 225, 
				abs(lower_arm_rotation - other.lower_arm_rotation) / 225, 
				abs(upper_arm_rotation - other.upper_arm_rotation) / 225,
			});
		}

		Position(double base_rotation, double lower_arm_rotation, double upper_arm_rotation, double hand_rotation, double wrist_rotation) :
		base_rotation(base_rotation),
		lower_arm_rotation(lower_arm_rotation),
		upper_arm_rotation(upper_arm_rotation),
		hand_rotation(hand_rotation),
		wrist_rotation(wrist_rotation)
		{}

		static Position Lerp(Position a, Position b, double t)
		{
			#define lrp(a,b,t) (a * (1-t) + b * t)
			Position out;
			out.base_rotation = lrp(a.base_rotation, b.base_rotation, t);
			out.lower_arm_rotation = lrp(a.lower_arm_rotation, b.lower_arm_rotation, t);
			out.upper_arm_rotation = lrp(a.upper_arm_rotation, b.upper_arm_rotation, t);
			out.hand_rotation = lrp(a.hand_rotation, b.hand_rotation, t);
			out.wrist_rotation = lrp(a.wrist_rotation, b.wrist_rotation, t);

			return out;
		}

		Position operator+(Position other)
		{
			Position copy = *this;
			copy.base_rotation += other.base_rotation;
			copy.lower_arm_rotation += other.lower_arm_rotation;
			copy.upper_arm_rotation += other.upper_arm_rotation;
			copy.hand_rotation += other.hand_rotation;
			copy.wrist_rotation += other.wrist_rotation;

			return copy;
		}

		Position operator-(Position other)
		{
			Position copy = *this;
			copy.base_rotation -= other.base_rotation;
			copy.lower_arm_rotation -= other.lower_arm_rotation;
			copy.upper_arm_rotation -= other.upper_arm_rotation;
			copy.hand_rotation -= other.hand_rotation;
			copy.wrist_rotation -= other.wrist_rotation;

			return copy;
		}
	};
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void SetupSerial();
	int NumOverlaps();
	void GetAnimation(Position position);
	void SendRotations();

	bool InverseKinematics(FVector target, Position& position);
	vector<Position> LinearMove(FVector current, FVector target, FVector impact_velocity, Position start_position = {0}, Position end_position = {0}, float delay_extra_position = 0, float delay_at_end = 0);
	void TrackParabola(Position& position);
	void TrackBall(FVector target, FVector impact_velocity, Position& position, FVector2d paddle_offset = {0,0});

	void ApplyPosition(Position position);

	void SerialLoop();
	FVector ArmOrigin() const;
	bool serial_loop_running = true;
	TFuture<void> serial_thread;
	double path_age = 10000000;
	ParabPath last_path;
	vector<Position> path_to_follow;

	Position GetPosition();
	Position GetActualPosition();

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
	
	UPROPERTY(EditAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	FString port;

	UPROPERTY(VisibleAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	int serial_port = -1;

	UPROPERTY(EditAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	bool debug_serial = false;

	UPROPERTY(EditAnywhere)
	AActor* robot_arm;
	
	UStaticMeshComponent *base_component;
	UStaticMeshComponent *lower_arm_component;
	UStaticMeshComponent *upper_arm_component;
	UStaticMeshComponent *hand_component;
	UStaticMeshComponent *wrist_component;
	
	UPROPERTY(EditAnywhere)
	bool show_profiling = false;
	
	// UPROPERTY(EditAnywhere)
	// AStaticMeshActor *wrist_component;
	
	UPROPERTY(EditAnywhere, Category = Motors)
	TEnumAsByte<UpdateType> update_type;
	
	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="IK Target", meta=(EditCondition = "update_type == UpdateType::IK", EditConditionHides))
	AActor* ik_target;

	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Path Target 1", meta=(EditCondition = "update_type == UpdateType::LinearPath", EditConditionHides))
	AActor* path_target1;
	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Path Target 2", meta=(EditCondition = "update_type == UpdateType::LinearPath", EditConditionHides))
	AActor* path_target2;
	
	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::LinearPath", EditConditionHides))
	bool switch_target = false;

	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::LinearPath", EditConditionHides))
	bool move_linearly = false;
	
	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	ABall* ball;

	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	bool draw_debug = false;

	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Lower arm length (m)", meta=(EditCondition = "update_type == UpdateType::IK || update_type == UpdateType::Ball || update_type == UpdateType::LinearPath", EditConditionHides))
	double lower_arm_length = 0.35;
	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Upper arm length (m)", meta=(EditCondition = "update_type == UpdateType::IK || update_type == UpdateType::Ball || update_type == UpdateType::LinearPath", EditConditionHides))
	double upper_arm_length = 0.265;

	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Hand length (m)", meta=(EditCondition = "update_type == UpdateType::Ball || update_type == UpdateType::LinearPath", EditConditionHides))
	double hand_length = 0.08;

	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="End offset (m)", meta=(EditCondition = "update_type == UpdateType::IK || update_type == UpdateType::Ball || update_type == UpdateType::LinearPath", EditConditionHides))
	double end_offset = 0.01;
	
	UPROPERTY(EditAnywhere, Category = Motors, DisplayName="Arm range (m)", meta=(EditCondition = "update_type == UpdateType::Ball || update_type == UpdateType::LinearPath", EditConditionHides))
	double arm_range = 0.6;
	
	
	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball || update_type == UpdateType::LinearPath", EditConditionHides))
	FVector impact;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-244.0", UIMax = "6.0", EditCondition = "update_type == UpdateType::User"))
	double base_rotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-90.0", UIMax = "90.0", EditCondition = "update_type == UpdateType::User"))
	double lower_arm_rotation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-225", UIMax = "33", EditCondition = "update_type == UpdateType::User"))
	double upper_arm_rotation;
    	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-75.0", UIMax = "95.0", EditCondition = "update_type == UpdateType::User"))
	double hand_rotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "0.0", UIMax = "180.0", EditCondition = "update_type == UpdateType::User"))
	double wrist_rotation;

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-244.0", UIMax = "6.0"))
	double actual_base_rotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-90.0", UIMax = "90.0"))
	double actual_lower_arm_rotation;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-225", UIMax = "33"))
	double actual_upper_arm_rotation;
    	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "-75.0", UIMax = "95.0"))
	double actual_hand_rotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Motors, meta=(UIMin = "0.0", UIMax = "180.0"))
	double actual_wrist_rotation;

};

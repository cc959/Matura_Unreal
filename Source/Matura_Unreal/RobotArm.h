// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <filesystem>

#include "GlobalIncludes.h"

#include "serial/serial.h"

#include "CoreMinimal.h"
#include "Ball.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "RobotArm.generated.h"

using namespace serial;

UENUM()
enum UpdateType
{
	User = 0,
	Animation = 1,
	IK = 2,
	Ball = 3,
	LinearPath = 4,
};

UENUM()
enum Tool
{
	Hoop = 0,
	Bat = 1,
};

#define NaN nan("")
#define isvalid(x) !(std::isnan(x) || std::isnan(-(x)))

UCLASS()
class MATURA_UNREAL_API ARobotArm : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARobotArm();

protected:
	static constexpr double motor_speed = 220; // degrees per second
	
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

		Position(double v = NaN)
		{
			base_rotation = lower_arm_rotation = upper_arm_rotation = hand_rotation = wrist_rotation = v;
		}

		double diff(Position other)
		{
			return max(max(
				abs(base_rotation - other.base_rotation) / motor_speed, 
				abs(lower_arm_rotation - other.lower_arm_rotation) / motor_speed), 
				abs(upper_arm_rotation - other.upper_arm_rotation) / motor_speed
			);
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
			copy.base_rotation += isvalid(other.base_rotation) ? other.base_rotation : 0;
			copy.lower_arm_rotation += isvalid(other.lower_arm_rotation) ? other.lower_arm_rotation : 0;
			copy.upper_arm_rotation += isvalid(other.upper_arm_rotation) ? other.upper_arm_rotation : 0;
			copy.hand_rotation += isvalid(other.hand_rotation) ? other.hand_rotation : 0;
			copy.wrist_rotation += isvalid(other.wrist_rotation) ? other.wrist_rotation : 0;

			return copy;
		}

		Position operator-(Position other)
		{
			Position copy = *this;
			copy.base_rotation -= isvalid(other.base_rotation) ? other.base_rotation : 0;
			copy.lower_arm_rotation -= isvalid(other.lower_arm_rotation) ? other.lower_arm_rotation : 0;
			copy.upper_arm_rotation -= isvalid(other.upper_arm_rotation) ? other.upper_arm_rotation : 0;
			copy.hand_rotation -= isvalid(other.hand_rotation) ? other.hand_rotation : 0;
			copy.wrist_rotation -= isvalid(other.wrist_rotation) ? other.wrist_rotation : 0;

			return copy;
		}

		Position operator^(Position other)
		{
			Position copy = *this;
			if (isvalid(other.base_rotation))
				copy.base_rotation = other.base_rotation;
			if (isvalid(other.lower_arm_rotation))
				copy.lower_arm_rotation = other.lower_arm_rotation;
			if (isvalid(other.upper_arm_rotation))
				copy.base_rotation = other.upper_arm_rotation;
			if (isvalid(other.hand_rotation))
				copy.hand_rotation = other.hand_rotation;
			if (isvalid(other.wrist_rotation))
				copy.wrist_rotation = other.wrist_rotation;

			return copy;
		}
	};

	struct LinearMove
	{
		ARobotArm* robot_arm = nullptr;

		FVector start, target;
		FVector impact_velocity;

		double min_duration = 0;
		
		Position rotation_before, rotation_after;
		double rotation_time = 0;

		bool absolute_rotation;

	private:
		double movement_time = 0;
		double start_time = -1;

	public:

		LinearMove() {}
		
		LinearMove(ARobotArm* robot_arm, FVector start, FVector target, FVector impact_velocity, double min_duration = 0, Position rotation_before = {0}, Position rotation_after = {0}, double rotation_time = 0, bool absolute_rotation = false)
			: robot_arm(robot_arm), start(start), target(target), impact_velocity(impact_velocity), min_duration(min_duration), rotation_before(rotation_before), rotation_after(rotation_after), rotation_time(rotation_time), absolute_rotation(absolute_rotation)
		{
			if (!robot_arm)
				return;
			
			Position start_rotation, target_rotation;
			
			robot_arm->TrackBall(start, impact_velocity, start_rotation);
			robot_arm->TrackBall(target, impact_velocity,  target_rotation);
			
			movement_time = start_rotation.diff(target_rotation);
		}
		
		bool operator()(double time, Position& position) {
			if (!robot_arm)
				return false;
			
			if (start_time == -1)
				start_time = time;

			time -= start_time;
			
			if (time > Length())
				return false;
			
			double t = min(time / movement_time, 1.);

			robot_arm->TrackBall(Lerp(start, target, t), impact_velocity, position);

			if (absolute_rotation)
			{
				if (time >= rotation_time)
				{
					position = position ^ rotation_after;
				}
				else
				{
					position = position ^ rotation_before;
				}
			}
			else
			{
				if (time >= rotation_time)
					position = position + rotation_after;
				else
					position = position + rotation_before;
			}

			return true;
		}

		void reset()
		{
			start_time = -1;
		}

		double Length() const
		{
			return max(min_duration, movement_time);
		}
	};
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void SetupSerial();
	int NumOverlaps();
	void GetAnimation(Position position);
	void SendRotations();

	bool InverseKinematics(FVector target, Position& position);
	void TrackParabola(Position& position);
	void TrackBall(FVector target, FVector impact_velocity, Position& position, FVector2d paddle_offset = {0,0});

	void ApplyPosition(Position position);

	void SerialLoop();
	FVector ArmOrigin() const;
	bool serial_loop_running = true;
	TFuture<void> serial_thread;
	double path_age = 10000000;
	ParabPath last_path;
	LinearMove path_to_follow;

	Position last_valid_position;
	Position ball_position;
	
	Position GetPosition();
	Position GetActualPosition();

	void BallLoop();
	bool RobotArmValid();
	void UpdateArm(float DeltaTime);
	void StopLoop();

	TFuture<void> ball_thread;

	bool ball_loop_running = false;
	int missed_ticks = 0;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	UPROPERTY(EditAnywhere)
	TEnumAsByte<Tool> tool;
	
	UPROPERTY(EditAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	FString port;

	//UPROPERTY(VisibleAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	Serial serial_port;

	UPROPERTY(EditAnywhere, Category = SerialSettings, meta = (EditCondition = "visual_only == false", EditConditionHides))
	bool debug_serial = false;

	UPROPERTY(EditAnywhere)
	AActor* robot_arm;
	
	UStaticMeshComponent *base_component;
	UStaticMeshComponent *lower_arm_component;
	UStaticMeshComponent *upper_arm_component;
	UStaticMeshComponent *hand_component;
	UStaticMeshComponent *wrist_component;
	UStaticMeshComponent *hoop_component;
	UStaticMeshComponent *bat_component;
	
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

	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	bool use_first_intersect = false;
	
	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	FVector aim_at = {-1400, 0, 2000};

	UPROPERTY(EditAnywhere, Category = Motors, meta=(EditCondition = "update_type == UpdateType::Ball", EditConditionHides))
	double outgoing_weight = 1;
	
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool replay_last_path;

};

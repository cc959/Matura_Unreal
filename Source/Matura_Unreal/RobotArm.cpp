// Fill out your copyright notice in the Description page of Project Settings.

#include "RobotArm.h"

// #include <fcntl.h>	 // Contains file controls like O_RDWR
// #include <errno.h>	 // Error integer and strerror() function
// #include <termios.h> // Contains POSIX terminal control definitions
// #include <unistd.h>	 // write(), read(), close()
#include <string>
#include <vector>
#include <cmath>
#include <Components/SphereComponent.h>

#include "Core/Public/Misc/AssertionMacros.h"

// Sets default values
ARobotArm::ARobotArm()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StopBallLoop();

	if (WITH_EDITOR)
	{
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bStartWithTickEnabled = true;
	}
}

// Called when the game starts or when spawned
void ARobotArm::BeginPlay()
{
	StopBallLoop();

	LogDisplay(TEXT("Robot arm started playing"));

	if (!visual_only)
		SetupSerial();

	Super::BeginPlay();
}

void ARobotArm::SetupSerial()
{
	// code sourced from https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
	std::string port_path(TCHAR_TO_UTF8(*port));

	serial_port.setPort(port_path);
	serial_port.setBaudrate(115200);
	try
	{
		serial_port.open();
	}
	catch (std::exception& e)
	{
		LogError(TEXT("Could not open serial port: %s"), *FString(e.what()));
	}
}

double CircularClamp(double& val, double low, double high, double step = 360)
{
	if (high < low)
		std::swap(low, high);

	if (abs(step) > 1e-6)
	{
		while (val < low)
			val += step;
		while (val > high)
			val -= step;
	}

	if (std::clamp(val, low, high) == val)
		return val;

	if (abs(fmod(low + step * 4, step) - fmod(val + step * 4, step)) < abs(fmod(high + step * 4, step) - fmod(val + step * 4, step)))
		val = low;
	else
		val = high;

	return val;
}

void ARobotArm::GetAnimation(Position position)
{
	if (base_component && base_component->GetAttachParent())
	{
		FRotator RelativeRotation = base_component->GetRelativeRotation();

		position.base_rotation = RelativeRotation.Euler().Z;
	}

	if (lower_arm_component && lower_arm_component->GetAttachParent())
	{
		FRotator RelativeRotation = lower_arm_component->GetRelativeRotation();

		position.lower_arm_rotation = RelativeRotation.Euler().X;
	}

	if (upper_arm_component && upper_arm_component->GetAttachParent())
	{
		FRotator RelativeRotation = upper_arm_component->GetRelativeRotation();

		position.upper_arm_rotation = RelativeRotation.Euler().X;
	}

	if (hand_component && hand_component->GetAttachParent())
	{
		FRotator RelativeRotation = hand_component->GetRelativeRotation();

		position.hand_rotation = RelativeRotation.Euler().X;
	}

	if (wrist_component && wrist_component->GetAttachParent())
	{
		FRotator RelativeRotation = wrist_component->GetRelativeRotation();

		position.wrist_rotation = RelativeRotation.Euler().Y;
	}

	CircularClamp(position.base_rotation, min_rotations[0], max_rotations[0]);
	CircularClamp(position.lower_arm_rotation, min_rotations[1], max_rotations[1]);
	CircularClamp(position.upper_arm_rotation, min_rotations[2], max_rotations[2]);
	CircularClamp(position.hand_rotation, min_rotations[3], max_rotations[3]);
	CircularClamp(position.wrist_rotation, min_rotations[4], max_rotations[4]);
}

void ARobotArm::SendRotations()
{
	auto [base_servo, lower_arm_servo, upper_arm_servo, hand_servo, wrist_servo] = GetPosition().to_servo().to_angle().to_servo();

	std::string msg = "> " + std::to_string(int(base_servo)) + " " + std::to_string(int(lower_arm_servo)) + " " +
		std::to_string(int(upper_arm_servo)) + " " + std::to_string(int(hand_servo)) + " " + std::to_string(int(wrist_servo)) + "\n";

	if (debug_serial)
	{
		FString blub = msg.c_str();
		LogDisplay(TEXT("Serial port: %s Message: %s"), *FString(serial_port.getPort().c_str()), *blub);
	}

	try
	{
		serial_port.write(msg);
		serial_port.flush();
	}
	catch (std::exception& e)
	{
		LogError(TEXT("Could not send message over serial port: %s"), *FString(e.what()));
	}
}

bool ARobotArm::InverseKinematics(FVector target, Position& position)
{
	FVector relative_position = target - arm_origin;

	// if (draw_debug)
	//	DrawDebugSphere(GetWorld(), target, 10, 10, FColor::Purple, false, -1, 1, 2);

	double offset = asin(end_offset * 100 * world_scale / FVector2d{relative_position.X, relative_position.Y}.Length());

	// UE Coordinate system is sus
	double plane_angle = atan2(relative_position.X, relative_position.Y) + offset + PI;

	bool must_flip = (plane_angle > max(-min_rotations[0], -max_rotations[0]) / 180 * PI || plane_angle < min(-min_rotations[0], -max_rotations[0]) /
		180 * PI);

	CircularClamp(plane_angle, -min_rotations[0] / 180 * PI, -max_rotations[0] / 180 * PI, PI);

	if (must_flip)
		plane_angle -= 2 * offset;

	position.base_rotation = -plane_angle * 180 / PI;

	FVector plane_up = {0, 0, 1};
	FVector plane_right = {sin(plane_angle), cos(plane_angle), 0};

	FVector2d plane_position = {relative_position.Dot(plane_right), relative_position.Dot(plane_up)};

	double a = lower_arm_length * 100 * world_scale;
	double b = upper_arm_length * 100 * world_scale;
	double c = plane_position.Length();

	double lower_arm_radians;
	double target_angle = atan2(plane_position.Y, plane_position.X);
	CircularClamp(target_angle, 0, 2 * PI, 2 * PI);

	if (a + b >= c)
	{
		lower_arm_radians = target_angle - acos((a * a + c * c - b * b) / (2 * a * c));
		double lower_arm_radians_2 = target_angle + acos((a * a + c * c - b * b) / (2 * a * c));

		if (abs(lower_arm_radians - PI / 2) > abs(lower_arm_radians_2 - PI / 2))
			std::swap(lower_arm_radians, lower_arm_radians_2);
	}
	else
	{
		lower_arm_radians = target_angle;
	}

	CircularClamp(lower_arm_radians, (-min_rotations[1] + 90) / 180 * PI, (-max_rotations[1] + 90) / 180 * PI, 2 * PI);

	FVector2d lower_arm_end = FVector2d{cos(lower_arm_radians), sin(lower_arm_radians)} * a;

	double upper_arm_radians = atan2(plane_position.Y - lower_arm_end.Y, plane_position.X - lower_arm_end.X) - lower_arm_radians;
	CircularClamp(upper_arm_radians, (-min_rotations[2] - 90) / 180 * PI, (-max_rotations[2] - 90) / 180 * PI, 2 * PI);

	position.lower_arm_rotation = -(lower_arm_radians / PI * 180 - 90);
	position.upper_arm_rotation = -(upper_arm_radians / PI * 180 + 90);

	return must_flip;
}

// angle for the ball to fly to minimize the starting velocity but still reach the point (length,height)
std::pair<double, double> best_angle(double length, double height, double g)
{
	double delta = 0.001;
	double learning_rate = 0.01;
	int max_iterations = 1000;

	double l = atan2(height, length), r = M_PI / 2;

	double x = (l + r) / 2;
	double prev_x = x - 2 * delta;

	auto v = [&](double angle)
	{
		return sqrt(g / 2 * length * length / (cos(angle) * cos(angle) * (height - tan(angle) * length)));
	};

	int i = 0;
	for (; i < max_iterations && std::abs(x - prev_x) > delta; i++)
	{
		prev_x = x;

		double right = v(x + delta);
		double left = v(x - delta);

		double gradient = (right - left) / (2 * delta);
		x -= learning_rate * gradient;
	}

	return {x, v(x)};
}

std::pair<FVector, FVector> best_impact(FVector v0, FVector v1)
{
	FVector normal = -(v0 - v1);
	double mag = normal.Length();
	normal /= mag;

	FVector v_bat = (0.5 * mag + v0.Dot(normal)) * normal;

	FVector v1_check = v0 - 2. * (v0 - v_bat).Dot(normal) * normal;

	double dot = abs(v1_check.Dot(v1) / v1_check.Length() / v1.Length() - 1);

	if (dot > 1e-6)
		LogError(TEXT("Something seems wrong with the best impact function: %f %f %f * %f %f %f = %f"), v1.X, v1.Y, v1.Z, v1_check.X, v1_check.Y,
	         v1_check.Z, dot);

	return {normal, v_bat};
}

void ARobotArm::TrackParabola(Position& position, double DeltaTime)
{
	ParabPath tracking_path;
	if (ball->started)
		ball->tracking_path.pop(&tracking_path);
	if (!ball || !tracking_path.IsValid())
	{
		if (path_age > 0.25)
		{
			// if last flight was more than 2s ago, go back to home position
			position = rest_position;
			return;
		}
		path_age += DeltaTime;
	}
	else
	{
		path_age = 0;
		last_path = tracking_path;
	}

	double intersection_radius = arm_range * 100 * world_scale;

	std::vector<double> intersections = last_path.IntersectSphere(arm_origin, intersection_radius);

	// only times in the future, not infinity and a number are valid
	intersections.erase(std::remove_if(intersections.begin(), intersections.end(),
	                                   [&](double p)
	                                   {
		                                   return p <= last_path.t1 + path_age || isnan(p) || isnan(-p) || isinf(p);
	                                   }),
	                    intersections.end());

	if (intersections.size() == 0)
		return;

	auto calculate_bat = [&](FVector target, FVector impact_velocity)
	{
		FVector aim = aim_at - target;
		double yaw_angle = atan2(aim.Y, -aim.X);

		auto [pitch_angle, v] = best_angle(sqrt(aim.X * aim.X + aim.Y * aim.Y), aim.Z, -9810);

		FVector dir = FVector(-v, 0, 0);

		dir = FQuat(FVector::RightVector, pitch_angle).Rotator().RotateVector(dir);
		dir = FQuat(FVector::UpVector, -yaw_angle).Rotator().RotateVector(dir);

		return best_impact(impact_velocity, dir);
	};

	if (intersections.size() == 1)
	{
		intersections.push_back(intersections[0]);
	}

	double intercept_time = -1;
	double best_score = std::numeric_limits<double>::infinity();

	for (double target_time = intersections[0]; target_time <= intersections[1]; target_time += max((intersections[1] - intersections[0]) / 20., 1e-2))
	{
		FVector target = last_path(target_time);
		FVector impact_velocity = {last_path.vx, last_path.vy, last_path.derivative(target_time)};

		if ((target - arm_origin).Z < 0)
			continue;

		if ((target - arm_origin).Length() < arm_range * 0.5)
			// don't want to intercept too close, otherwise not enough freedom to play the ball back
			continue;
		
		if (tool == Bat)
		{
			Position candidate;
			auto [normal, v_bat] = calculate_bat(target, impact_velocity);
			TrackBall(target, -normal, candidate);
			candidate.hand_rotation -= 20;
		
			if (!CheckCollision(candidate)) // discard paths that result in a collision
				continue;
		}

		Position candidate;
		TrackBall(target, impact_velocity, candidate);

		double est_move_time = candidate.diff(GetActualPosition());

		if (est_move_time * 1.5 > abs(target_time - (last_path.t1 + path_age)))
			// discard paths that would take too long, mostly these are false detections
			continue;

		if (!CheckCollision(candidate)) // discard paths that result in a collision
			continue;

		if (est_move_time - target_time < best_score) // move time should be small and should intercept rather late in the path
		{
			intercept_time = target_time;
			best_score = est_move_time - target_time;
		}
	}

	if (intercept_time < 0)
		return;
	

	FVector target = last_path(intercept_time);
	FVector impact_velocity = {last_path.vx, last_path.vy, last_path.derivative(intercept_time)};

	if (tool == Bat)
	{
		auto [normal, v_bat] = calculate_bat(target, impact_velocity);
		TrackBall(target, -normal, position);

#if WITH_EDITOR // latency differs in in editor and in packaged/standalone build
		if (abs(intercept_time - (last_path.t1 + path_age)) < 0.325)
#else
		if (abs(intercept_time - (last_path.t1 + path_age)) < 0.300)
#endif
		{
			Position start{NaN, NaN, NaN, position.hand_rotation - 20, NaN};
			Position end{NaN, NaN, NaN, position.hand_rotation + 10, NaN};

			path_to_follow = LinearMove(this, last_path(intercept_time), last_path(intercept_time) + v_bat * (150 / v_bat.Length()), -normal, 0.4,
			                            start, end, 0.1, true);
			path_age += path_to_follow.Length();
		}
		else
		{
			position.hand_rotation -= 20;
		}
	}
	else
	{
		TrackBall(target, impact_velocity, position);
	}
}

void ARobotArm::TrackBall(FVector target, FVector impact_velocity, Position& position, FVector2d paddle_offset)
{
	impact_velocity.Normalize();

	FVector relative_position = target - arm_origin;

	bool fixed = false;
	double offset = asin(end_offset * 100 * world_scale / FVector2d{relative_position.X, relative_position.Y}.Length());

	double plane_angle = atan2(relative_position.X, relative_position.Y) + offset;

again:

	auto base_rotator = FQuat(FVector::UpVector, -plane_angle).Rotator();
	FVector impact_velocity_plane = base_rotator.UnrotateVector(impact_velocity);

	FVector pitch_down = impact_velocity_plane * (FVector::UpVector + FVector::RightVector);
	pitch_down.Normalize();
	double pitch_angle = atan2(-pitch_down.Y, pitch_down.Z);

	auto hand_rotator = FQuat(FVector::ForwardVector, pitch_angle).Rotator();
	FVector roll_down = hand_rotator.UnrotateVector(impact_velocity_plane) * (FVector::UpVector + FVector::ForwardVector);
	roll_down.Normalize();
	double roll_angle = atan2(roll_down.X, roll_down.Z);

	FVector arm_offset = FVector{0, cos(pitch_angle), sin(pitch_angle)} * (hand_length * 100 * world_scale);

	FVector paddle_y = base_rotator.RotateVector(arm_offset) / arm_offset.Length();
	FVector paddle_x = paddle_y.Cross(impact_velocity / impact_velocity.Length());

	if (!paddle_offset.IsNearlyZero())
	{
		FVector ball_offset = -(paddle_offset.X * paddle_x + paddle_offset.Y * paddle_y);
		TrackBall(target + ball_offset, impact_velocity, position);
		return;
	}

	bool flipped = InverseKinematics(target + base_rotator.RotateVector(arm_offset), position);
	if (flipped && !fixed)
	{
		plane_angle -= 2 * offset;
		fixed = true;
		goto again;
	}

	position.hand_rotation = (flipped ? PI - pitch_angle : pitch_angle) / PI * 180 - position.lower_arm_rotation - position.upper_arm_rotation;
	position.wrist_rotation = roll_angle / PI * 180 + 90;

	CircularClamp(position.hand_rotation, min_rotations[3], max_rotations[3]);
	CircularClamp(position.wrist_rotation, min_rotations[4], max_rotations[4]);
}

bool ARobotArm::CheckCollision(Position position)
{
	position = GetPosition() ^ position;

	double base_angle = -position.base_rotation / 180 * PI;
	double lower_arm_angle = (position.lower_arm_rotation + 90) / 180 * PI;
	double upper_arm_angle = (position.upper_arm_rotation + 90) / 180 * PI + lower_arm_angle;
	double hand_angle = position.hand_rotation / 180 * PI + upper_arm_angle;
	double wrist_angle = (position.wrist_rotation - 90) / 180 * PI;

	double tool_length = hand_length + (tool == Bat ? 0.04 : 0.06);
	double tool_width = (tool == Bat ? 0.07 : 0.05);

	FRotator base_rotator(0, base_angle * 180 / PI, 0);
	FRotator wrist_rotator(-wrist_angle / PI * 180, 0, 0);
	FRotator hand_rotator(0, 0, -hand_angle / PI * 180);

	FVector lower_arm_dir = base_rotator.RotateVector({0, cos(lower_arm_angle) * lower_arm_length, sin(lower_arm_angle) * lower_arm_length});
	FVector upper_arm_dir = base_rotator.RotateVector({0, cos(upper_arm_angle) * upper_arm_length, sin(upper_arm_angle) * upper_arm_length});

	FVector hand_right = base_rotator.RotateVector(hand_rotator.RotateVector(wrist_rotator.RotateVector({1, 0, 0})));
	FVector hand_forward = base_rotator.RotateVector(hand_rotator.RotateVector(wrist_rotator.RotateVector({0, 1, 0})));

	FVector base_right = base_rotator.RotateVector(FVector{1, 0, 0});

	auto is_invalid = [&](double t, double mult = 1)
	{
		double upper_arm_t = clamp(t, 0., 1.1);
		double hand_t = clamp(t - 1.1, 0., 1.);

		if (hand_t > 0)
			upper_arm_t = 1, hand_t += 0.1;

		FVector eval = lower_arm_dir + upper_arm_dir * upper_arm_t;

		if (t > 1.1)
		{
			eval += hand_right * sin(acos(clamp(2 * pow(hand_t, 1.5) - 1, -1., 1.))) * tool_width * mult;
			eval += hand_forward * tool_length * hand_t;
		}
		else
		{
			eval += base_right * 0.05;
		}

		eval.Y *= -1;
		eval.Z += arm_origin.Z / 1000;

		//DrawDebugSphere(GetWorld(), eval * 1000, 10, 2, FColor::Blue, false, -1, 2, 1);

		double delta = 0;
		if (t <= 1)
			delta = 0.025;
		else if (t <= 2)
			delta = 0.01;

		double rad = eval.X * eval.X + eval.Y * eval.Y;

		bool under_plate = eval.Z < base_plate_height + delta && abs(eval.X) < base_plate_x / 2 + delta && abs(eval.Y) < base_plate_y / 2 + delta;
		bool inside_base = rad < pow(base_radius + delta, 2) && eval.Z < base_height + base_plate_height + delta;

		return under_plate || inside_base;
	};

	for (double t = 0; t <= 2.05; t += 0.1)
	{
		if (t > 1.1)
		{
			if (is_invalid(t, -1) || is_invalid(t, 1) || is_invalid(t, 0))
				return false;
		}
		else
		{
			if (is_invalid(t))
				return false;
		}
	}
	return true;
}

bool ARobotArm::ApplyPosition(Position position)
{
	if (CheckCollision(position))
	{
		position = GetPosition() ^ position;
		base_rotation = position.base_rotation;
		lower_arm_rotation = position.lower_arm_rotation;
		upper_arm_rotation = position.upper_arm_rotation;
		hand_rotation = position.hand_rotation;
		wrist_rotation = position.wrist_rotation;
		last_valid_position = position;

		return true;
	}
	return false;
}

UActorComponent* FirstWithTag(AActor* actor, FName tag)
{
	auto components = actor->GetComponentsByTag(UStaticMeshComponent::StaticClass(), tag);
	return components.Num() == 0 ? nullptr : components[0];
}

ARobotArm::Position ARobotArm::GetPosition()
{
	return Position{base_rotation, lower_arm_rotation, upper_arm_rotation, hand_rotation, wrist_rotation};
}

ARobotArm::Position ARobotArm::GetActualPosition()
{
	return Position{actual_base_rotation, actual_lower_arm_rotation, actual_upper_arm_rotation, actual_hand_rotation, actual_wrist_rotation};
}

void ARobotArm::BallLoop()
{
	LogDisplay(TEXT("Started robot arm catch loop"));

	auto last_tick = NOW;
	auto start_tick = NOW;

	bool using_path = false;

	while (ball_loop_running)
	{
		auto tick = NOW;
		double DeltaTime = (tick - last_tick).count() / 1e9;
		last_tick = tick;

		double now = (tick - start_tick).count() / 1e9;

		if (replay_last_path)
		{
			replay_last_path = false;
			path_to_follow.reset();
		}

		Position new_position;
		if (!path_to_follow(now, new_position))
		{
			TrackParabola(new_position, DeltaTime);
			if (using_path)
				new_position = rest_position ^ new_position;
			using_path = false;
		}

		ApplyPosition(new_position);

		actual_base_rotation = actual_base_rotation + std::clamp(base_rotation - actual_base_rotation, -motor_speed * DeltaTime,
		                                                         motor_speed * DeltaTime);
		actual_lower_arm_rotation = actual_lower_arm_rotation + std::clamp(lower_arm_rotation - actual_lower_arm_rotation, -motor_speed * DeltaTime,
		                                                                   motor_speed * DeltaTime);
		actual_upper_arm_rotation = actual_upper_arm_rotation + std::clamp(upper_arm_rotation - actual_upper_arm_rotation, -motor_speed * DeltaTime,
		                                                                   motor_speed * DeltaTime);
		actual_hand_rotation = hand_rotation;
		actual_wrist_rotation = wrist_rotation;

		if (!visual_only)
		{
			SendRotations();
		}

		if (show_profiling)
			LogDisplay(TEXT("Ball Loop running at %f fps, %f ms between update"), 1. / DeltaTime, DeltaTime * 1000.);

		if (path_to_follow(now, new_position))
		{
			usleep(2000); // sleep 2ms
			using_path = true;
		}
	}

	LogDisplay(TEXT("Stopped robot arm catch loop"));
	path_to_follow = {};
}

bool ARobotArm::RobotArmValid()
{
	if (robot_arm)
	{
		base_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Base"));
		lower_arm_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Lower_Arm"));
		upper_arm_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Upper_Arm"));
		hand_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Hand"));
		wrist_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Wrist"));
		hoop_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Hoop"));
		bat_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Bat"));

		if (!base_component)
			LogError(TEXT("Robot arm has nothing with tag \"Base\""));
		if (!lower_arm_component)
			LogError(TEXT("Robot arm has nothing with tag \"Lower_Arm\""));
		if (!upper_arm_component)
			LogError(TEXT("Robot arm has nothing with tag \"Upper_Arm\""));
		if (!hand_component)
			LogError(TEXT("Robot arm has nothing with tag \"Hand\""));
		if (!wrist_component)
			LogError(TEXT("Robot arm has nothing with tag \"Wrist\""));
		if (!hoop_component)
			LogError(TEXT("Robot arm has nothing with tag \"Hoop\""));
		if (!bat_component)
			LogError(TEXT("Robot arm has nothing with tag \"Bat\""));

		if (!base_component)
			return false;
		if (!lower_arm_component)
			return false;
		if (!upper_arm_component)
			return false;
		if (!hand_component)
			return false;
		if (!wrist_component)
			return false;
		if (!hoop_component)
			return false;
		if (!bat_component)
			return false;

		if (hoop_component)
		{
			if (tool == Hoop)
				hoop_component->SetRelativeScale3D(FVector(1));
			else
				hoop_component->SetRelativeScale3D(FVector(0.01));
		}

		if (bat_component)
		{
			if (tool == Bat)
				bat_component->SetRelativeScale3D(FVector(1));
			else
				bat_component->SetRelativeScale3D(FVector(0.01));
		}

		return true;
	}
	return false;
}

void ARobotArm::UpdateArmSyncronous(float DeltaTime)
{
	if (update_rotations)
	{
		Position new_position;

		double now = UGameplayStatics::GetRealTimeSeconds(GetWorld());

		if (replay_last_path)
		{
			replay_last_path = false;
			path_to_follow.reset();
		}

		if (!path_to_follow(now, new_position))
		{
			if (update_type == User)
			{
				new_position = {base_rotation, lower_arm_rotation, upper_arm_rotation, hand_rotation, wrist_rotation};
				if (isvalid(last_valid_position.base_rotation))
					base_rotation = last_valid_position.base_rotation;
				if (isvalid(last_valid_position.lower_arm_rotation))
					lower_arm_rotation = last_valid_position.lower_arm_rotation;
				if (isvalid(last_valid_position.upper_arm_rotation))
					upper_arm_rotation = last_valid_position.upper_arm_rotation;
				if (isvalid(last_valid_position.hand_rotation))
					hand_rotation = last_valid_position.hand_rotation;
				if (isvalid(last_valid_position.wrist_rotation))
					wrist_rotation = last_valid_position.wrist_rotation;
			}

			if (update_type == Animation)
				GetAnimation(new_position);

			if (update_type == LinearPath)
			{
				if (path_target1 && path_target2)
				{
					TrackBall(path_target1->GetActorLocation(), impact, new_position);

					if (switch_target)
					{
						if (move_linearly)
							path_to_follow = LinearMove(this, path_target1->GetActorLocation(), path_target2->GetActorLocation(), impact);
						swap(path_target1, path_target2);
						switch_target = false;
					}
				}
			}

			if (update_type == IK && ik_target)
				InverseKinematics(ik_target->GetActorLocation(), new_position);
		}

		ApplyPosition(new_position);

		actual_base_rotation = actual_base_rotation + std::clamp(base_rotation - actual_base_rotation, -motor_speed * DeltaTime,
		                                                         motor_speed * DeltaTime);
		actual_lower_arm_rotation = actual_lower_arm_rotation + std::clamp(lower_arm_rotation - actual_lower_arm_rotation, -motor_speed * DeltaTime,
		                                                                   motor_speed * DeltaTime);
		actual_upper_arm_rotation = actual_upper_arm_rotation + std::clamp(upper_arm_rotation - actual_upper_arm_rotation, -motor_speed * DeltaTime,
		                                                                   motor_speed * DeltaTime);
		actual_hand_rotation = hand_rotation;
		actual_wrist_rotation = wrist_rotation;

		if (!visual_only && GetWorld()->IsGameWorld())
		{
			SendRotations();
		}
	}
}

void ARobotArm::StopBallLoop()
{
	ball_loop_running = false;
	if (ball_thread.IsValid())
		ball_thread.Wait();
}

// Called every frame
void ARobotArm::Tick(float DeltaTime)
{
	if (RobotArmValid())
	{
		arm_origin = FVector{
			base_component->GetComponentLocation().X,
			base_component->GetComponentLocation().Y,
			lower_arm_component->GetComponentLocation().Z
		};

		world_scale = base_component->GetComponentScale().X;
		
		if (update_type != Ball)
		{
			StopBallLoop();
			UpdateArmSyncronous(DeltaTime);
		}
		else
		{
			if (GetWorld()->IsGameWorld())
			{
				if (!ball_loop_running)
				{
					StopBallLoop();
					LogDisplay(TEXT("Started robot arm loop"));
					ball_loop_running = true;
					Async(EAsyncExecution::Thread, [&]
					{
						BallLoop();
					});
				}
			}
			else
			{
				ApplyPosition(rest_position);
				actual_base_rotation = actual_base_rotation + std::clamp(base_rotation - actual_base_rotation, -motor_speed * DeltaTime,
																		 motor_speed * DeltaTime);
				actual_lower_arm_rotation = actual_lower_arm_rotation + std::clamp(lower_arm_rotation - actual_lower_arm_rotation,
																				   -motor_speed * DeltaTime, motor_speed * DeltaTime);
				actual_upper_arm_rotation = actual_upper_arm_rotation + std::clamp(upper_arm_rotation - actual_upper_arm_rotation,
																				   -motor_speed * DeltaTime, motor_speed * DeltaTime);
				actual_hand_rotation = hand_rotation;
				actual_wrist_rotation = wrist_rotation;
			}
		}

		if (base_component)
			base_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, 0, actual_base_rotation)));
		if (lower_arm_component)
			lower_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(actual_lower_arm_rotation, 0, 0)));
		if (upper_arm_component)
			upper_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(actual_upper_arm_rotation, 0, 0)));
		if (hand_component)
			hand_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(actual_hand_rotation, 0, 0)));
		if (wrist_component)
			wrist_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, actual_wrist_rotation, 0)));
	}
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void ARobotArm::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool ARobotArm::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif

void ARobotArm::BeginDestroy()
{
	Super::BeginDestroy();

	serial_port.close();

	StopBallLoop();
}

void ARobotArm::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	serial_port.close();

	StopBallLoop();
}

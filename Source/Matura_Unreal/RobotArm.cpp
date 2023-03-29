// Fill out your copyright notice in the Description page of Project Settings.

#include "RobotArm.h"

#include <fcntl.h>	 // Contains file controls like O_RDWR
#include <errno.h>	 // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>	 // write(), read(), close()
#include <string>
#include <vector>
#include "Core/Public/Misc/AssertionMacros.h"

// Sets default values
ARobotArm::ARobotArm()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	if (WITH_EDITOR)
	{
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bStartWithTickEnabled = true;
	}
}

// Called when the game starts or when spawned
void ARobotArm::BeginPlay()
{
	serial_port = -1;

	if (!visual_only)
		SetupSerial();

	Super::BeginPlay();
}

void ARobotArm::SetupSerial()
{
	// code sourced from https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
	std::string port_path(TCHAR_TO_UTF8(*port));

	serial_port = open(port_path.c_str(), O_RDWR);

	if (serial_port < 0)
	{
		FString errorText(strerror(errno));
		UE_LOG(LogTemp, Error, TEXT("Error from open: %s"), *errorText);
		return;
	}

	struct termios tty;

	if (tcgetattr(serial_port, &tty) != 0)
	{
		FString errorText(strerror(errno));
		UE_LOG(LogTemp, Error, TEXT("Error from tcgetattr: %s"), *errorText);
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag &= ~CSIZE; // Clear all the size bits, then use one of the statements below
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_lflag &= ~ICANON;
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo

	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
		ICRNL); // Disable any special wristling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)

	tty.c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0; // return after reading at least VMIN bytes

	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);

	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
	{
		FString errorText(strerror(errno));
		UE_LOG(LogTemp, Display, TEXT("Error from tcsetattr: %s"), *errorText);
	}

	serial_thread = Async(EAsyncExecution::Thread, [&]
	{
		SerialLoop();
	});
}

int ARobotArm::NumOverlaps()
{
	if (!robot_arm)
		return 0;

	int cnt = 0;

	TArray colliders = robot_arm->GetComponentsByTag(UStaticMeshComponent::StaticClass(), "Collider");
	for (auto collider : colliders)
	{
		if (collider && Cast<UStaticMeshComponent>(collider))
		{
			TArray<UPrimitiveComponent*> overlaps;
			Cast<UStaticMeshComponent>(collider)->GetOverlappingComponents(overlaps);
			cnt += overlaps.Num();
		}
	}

	return cnt;
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


double interpolate(double x, double a, double b, double c, double d)
{
	double t = (x - a) / (b - a);
	return c + (d - c) * t;
}

void ARobotArm::SendRotations()
{
	double base_servo =
		interpolate(base_rotation, min_rotations[0], max_rotations[0], min_servo[0], max_servo[0]);
	double lower_arm_servo =
		interpolate(lower_arm_rotation, min_rotations[1], max_rotations[1], min_servo[1], max_servo[1]);
	double upper_arm_servo =
		interpolate(upper_arm_rotation, min_rotations[2], max_rotations[2], min_servo[2], max_servo[2]);
	double hand_servo =
		interpolate(hand_rotation, min_rotations[3], max_rotations[3], min_servo[3], max_servo[3]);
	double wrist_servo =
		interpolate(wrist_rotation, min_rotations[4], max_rotations[4], min_servo[4], max_servo[4]);

	std::string msg = "> " + std::to_string(int(base_servo)) + " " + std::to_string(int(lower_arm_servo)) + " " +
		std::to_string(int(upper_arm_servo)) + " " + std::to_string(int(hand_servo)) + " " + std::to_string(int(wrist_servo)) + "\n";

	if (debug_serial)
	{
		FString blub = msg.c_str();
		UE_LOG(LogTemp, Display, TEXT("Serial port: %d Message: %s"), serial_port, *blub);
	}

	if (serial_port < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Serial port is not open, can't send data!"));
		return;
	}

	write(serial_port, &msg[0], sizeof(msg[0]) * msg.size());
	tcdrain(serial_port);
	tcflush(serial_port, TCIOFLUSH);
}

void ARobotArm::SerialLoop()
{
	while (serial_loop_running)
	{
		SendRotations();
		usleep(10000); // sleep for 10ms
	}

	close(serial_port);
}

FVector ARobotArm::ArmOrigin() const
{
	return FVector
	{
		base_component->GetComponentLocation().X,
		base_component->GetComponentLocation().Y,
		lower_arm_component->GetComponentLocation().Z
	};
}

bool ARobotArm::InverseKinematics(FVector target, Position& position)
{
	if (!base_component || !lower_arm_component || !upper_arm_component)
		return false;

	FVector relative_position = target - ArmOrigin();

	DrawDebugSphere(GetWorld(), target, 10, 10, FColor::Purple, false, -1, 1, 2);

	double offset = asin(end_offset * 100 * base_component->GetComponentScale().X / FVector2d{relative_position.X, relative_position.Y}.Length());

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

	double a = lower_arm_length * 100 * lower_arm_component->GetComponentScale().X;
	double b = upper_arm_length * 100 * upper_arm_component->GetComponentScale().X;
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
pair<double, double> best_angle(double length, double height, double g)
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

pair<FVector, FVector> best_impact(FVector v0, FVector v1)
{
	FVector normal = -(v0 - v1);
	double mag = normal.Length();
	normal /= mag;

	FVector v_bat = 0.5 * mag * normal + v0;

	FVector v1_check = v0 - 2. * (v0 - v_bat).Dot(normal) * normal;

	double dot = abs(v1_check.Dot(v1) / v1_check.Length() / v1.Length() - 1);

	if (dot > 1e-6)
		UE_LOG(LogTemp, Error, TEXT("Something seems wrong with the best impact function: %f %f %f * %f %f %f = %f"), v1.X, v1.Y, v1.Z, v1_check.X, v1_check.Y, v1_check.Z, dot);
	
	return {normal, v_bat};
}

void ARobotArm::TrackParabola(Position& position)
{
	if (!ball || !ball->tracking_path.IsValid() || !base_component || !wrist_component || !hand_component)
	{
		if (path_age > 0.25)
		{
			// if last flight was more than 2s ago, go back to home position
			position = Position{-90, -30, -200, 0, 90};
			return;
		}
		path_age += GetWorld()->GetDeltaSeconds();
	}
	else
	{
		path_age = 0;
		last_path = ball->tracking_path;
	}
	
	double intersection_radius = arm_range * 100 * base_component->GetComponentScale().X;
	
	
	vector<double> intersections = last_path.IntersectSphere(ArmOrigin(), intersection_radius);
	
	// only times in the future, not infinity and a number are valid - obviously
	intersections.erase(remove_if(intersections.begin(), intersections.end(),
	                              [&](double p)
	                              {
		                              return p <= last_path.t1 + path_age || isnan(p) || isinf(p);
	                              }), intersections.end());
	
	if (intersections.size() == 0)
	{
		return;
	}
	
	if (draw_debug)
		for (auto t : intersections)
		{
			DrawDebugSphere(GetWorld(), last_path(t), intersection_radius / 20, 10, FColor::Purple, 0, -1, 1, 3);
			UE_LOG(LogTemp, Display, TEXT("Intersection at %f"), t);
		}
	UE_LOG(LogTemp, Display, TEXT("Now at %f"), last_path.t1+path_age);
	
	
	double intercept_time = intersections[0];
	
	double best_score = 1e69; // infinity
	
	if (intersections.size() >= 2)
	{
		for (double target_time = intersections[0]; target_time < intersections[1]; target_time += (intersections[1] - intersections[0]) / 20.)
		{
			FVector target = last_path(target_time);
			FVector impact_velocity = {last_path.vx, last_path.vy, last_path.derivative(target_time)};
	
			if ((target - ArmOrigin()).Z < 0)
				continue;
	
			if ((target - ArmOrigin()).Length() < arm_range * 0.5)
				// don't want to intercept too close, otherwise not enough freedom to play the ball back
				continue;
	
			Position candidate;
			TrackBall(target, impact_velocity, candidate);
	
			double est_move_time = candidate.diff(GetActualPosition());
	
			if (est_move_time * 1.5 > abs(target_time - (last_path.t1 + path_age)))
				// discard paths that would take too long, mostly these are false detections
				continue;
	
			if (est_move_time - target_time < best_score) // move time should be small and should intercept rather late in the path
			{
				intercept_time = target_time;
				best_score = est_move_time - target_time;
			}
		}
	}
	
	if (draw_debug)
		DrawDebugSphere(GetWorld(), ArmOrigin(), intersection_radius, 100,
		                (intercept_time - (last_path.t1 + path_age) < 0.3) ? FColor::Red : FColor::Blue, 0, -1);
	
	// if (!ik_target)
	// 	return;

	FVector aim_at(-1400, 0, 800);

	FVector target = last_path(intercept_time);
	FVector impact_velocity = {last_path.vx, last_path.vy, last_path.derivative(intercept_time)};
	impact_velocity.Normalize();

	FVector aim = aim_at - target;
	double yaw_angle = atan2(aim.Y, -aim.X);

	auto [pitch_angle, v] = best_angle(sqrt(aim.X * aim.X + aim.Y * aim.Y), aim.Z, -9810);

	FVector dir = FVector(-v, 0, 0);

	dir = FQuat(FVector::RightVector, pitch_angle).Rotator().RotateVector(dir);
	dir = FQuat(FVector::UpVector, -yaw_angle).Rotator().RotateVector(dir);

	for (double t = 0; t < 1; t += 0.01)
	{
		DrawDebugLine(GetWorld(), target + dir * t + FVector(0, 0, -9810) / 2. * t * t,
		              target + dir * (t + 0.05) + FVector(0, 0, -9810) / 2. * (t + 0.05) * (t + 0.05), FColor::Red, false, -1, 1, 10);
	}
	DrawDebugLine(GetWorld(), target, target - impact_velocity, FColor::Red, false, -1, 1, 10);

	auto [normal, v_bat] = best_impact(impact_velocity, dir);

	DrawDebugLine(GetWorld(), target, target + v_bat, FColor::Green, false, -1, 2, 10);

	TrackBall(target, -normal, position, {0, 0});

	if (abs(intercept_time - (last_path.t1 + path_age)) < 0.4)
	{
		Position start{0, 0, 0, -25, 0};
		Position end{0, 0, 0, 15, 0};
	
		path_to_follow = LinearMove(this, last_path(intercept_time), last_path(intercept_time) + v_bat * (150 / v_bat.Length()), -normal, 0.4,  start, end, 0.1);
		path_age += path_to_follow.Length();
	}
	else
	{
		position.hand_rotation -= 30;
	}
}

void ARobotArm::TrackBall(FVector target, FVector impact_velocity, Position& position, FVector2d paddle_offset)
{
	FVector relative_position = target - ArmOrigin();

	bool fixed = false;
	double offset = asin(end_offset * 100 * base_component->GetComponentScale().X / FVector2d{relative_position.X, relative_position.Y}.Length());

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

	FVector arm_offset = FVector{0, cos(pitch_angle), sin(pitch_angle)} * (hand_length * 100 * hand_component->GetComponentScale().X);

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

void ARobotArm::ApplyPosition(Position position)
{
	if (base_component && lower_arm_component && upper_arm_component && hand_component && wrist_component)
	{
		int before = NumOverlaps();
		{
			if (!isnan(position.base_rotation))
				base_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, 0, position.base_rotation)));

			if (!isnan(position.lower_arm_rotation))
				lower_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(position.lower_arm_rotation, 0, 0)));

			if (!isnan(position.upper_arm_rotation))
				upper_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(position.upper_arm_rotation, 0, 0)));

			if (!isnan(position.hand_rotation))
				hand_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(position.hand_rotation, 0, 0)));

			if (!isnan(position.wrist_rotation))
				wrist_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, position.wrist_rotation, 0)));
		}
		int after = NumOverlaps();

		if (after > before && check_collision)
		{
			base_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, 0, base_rotation)));
			lower_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(lower_arm_rotation, 0, 0)));
			upper_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(upper_arm_rotation, 0, 0)));
			hand_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(hand_rotation, 0, 0)));
			wrist_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, wrist_rotation, 0)));
		}
		else
		{
			if (!isnan(position.base_rotation)) base_rotation = position.base_rotation;
			if (!isnan(position.lower_arm_rotation)) lower_arm_rotation = position.lower_arm_rotation;
			if (!isnan(position.upper_arm_rotation)) upper_arm_rotation = position.upper_arm_rotation;
			if (!isnan(position.hand_rotation)) hand_rotation = position.hand_rotation;
			if (!isnan(position.wrist_rotation)) wrist_rotation = position.wrist_rotation;
		}
	}
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

// Called every frame
void ARobotArm::Tick(float DeltaTime)
{
	if (robot_arm)
	{
		base_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Base"));
		lower_arm_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Lower_Arm"));
		upper_arm_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Upper_Arm"));
		hand_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Hand"));
		wrist_component = Cast<UStaticMeshComponent>(FirstWithTag(robot_arm, "Wrist"));

		if (!base_component) UE_LOG(LogTemp, Error, TEXT("Robot arm has nothing with tag \"Base\""));
		if (!lower_arm_component) UE_LOG(LogTemp, Error, TEXT("Robot arm has nothing with tag \"Lower_Arm\""));
		if (!upper_arm_component) UE_LOG(LogTemp, Error, TEXT("Robot arm has nothing with tag \"Upper_Arm\""));
		if (!hand_component) UE_LOG(LogTemp, Error, TEXT("Robot arm has nothing with tag \"Hand\""));
		if (!wrist_component) UE_LOG(LogTemp, Error, TEXT("Robot arm has nothing with tag \"Wrist\""));
	}

	auto before = chrono::high_resolution_clock::now().time_since_epoch();
	if (update_rotations)
	{
		Position new_position;

		double now = UGameplayStatics::GetRealTimeSeconds(GetWorld());
		
		if (!path_to_follow(now, new_position))
		{
			if (update_type == Animation)
				GetAnimation(new_position);

			if (update_type == IK && ik_target)
				InverseKinematics(ik_target->GetActorLocation(), new_position);

			if (update_type == Ball)
				TrackParabola(new_position);

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
		}
		ApplyPosition(new_position);

		actual_base_rotation = actual_base_rotation
			+ clamp(base_rotation - actual_base_rotation, -motor_speed * DeltaTime, motor_speed * DeltaTime);
		actual_lower_arm_rotation = actual_lower_arm_rotation
			+ clamp(lower_arm_rotation - actual_lower_arm_rotation, -motor_speed * DeltaTime, motor_speed * DeltaTime);
		actual_upper_arm_rotation = actual_upper_arm_rotation
			+ clamp(upper_arm_rotation - actual_upper_arm_rotation, -motor_speed * DeltaTime, motor_speed * DeltaTime);
		actual_hand_rotation = hand_rotation;
		actual_wrist_rotation = wrist_rotation;

		if (base_component) base_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, 0, actual_base_rotation)));
		if (lower_arm_component) lower_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(actual_lower_arm_rotation, 0, 0)));
		if (upper_arm_component) upper_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(actual_upper_arm_rotation, 0, 0)));
		if (hand_component) hand_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(actual_hand_rotation, 0, 0)));
		if (wrist_component) wrist_component->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, actual_wrist_rotation, 0)));
	}
	auto after = chrono::high_resolution_clock::now().time_since_epoch();

	if (show_profiling)
		UE_LOG(LogTemp, Display, TEXT("Took %f ms to update robot arm"), (after - before).count() / 1e6);

	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void ARobotArm::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
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

	serial_loop_running = false;
	if (serial_thread.IsValid())
		serial_thread.Wait();
}

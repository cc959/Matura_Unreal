// Fill out your copyright notice in the Description page of Project Settings.

#include "RobotArm.h"

#include <fcntl.h>	 // Contains file controls like O_RDWR
#include <errno.h>	 // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>	 // write(), read(), close()
#include <string>
#include <vector>

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
		ICRNL); // Disable any special handling of received bytes

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
	int cnt = 0;

	for (auto collider : colliders)
	{
		if (collider)
		{
			TArray<AActor*> overlaps;
			collider->GetOverlappingActors(overlaps);
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


void ARobotArm::UpdateRotations()
{
	if (update_type == Animation)
	{
		if (base_component && base_component->GetAttachParentActor())
		{
			FRotator ActorRotation = base_component->GetActorRotation();
			FRotator parentRotation = base_component->GetAttachParentActor()->GetActorRotation();

			base_rotation = (ActorRotation - parentRotation).Euler().Z;
		}

		if (lower_arm_component && lower_arm_component->GetAttachParentActor())
		{
			FRotator ActorRotation = lower_arm_component->GetActorRotation();
			FRotator parentRotation = lower_arm_component->GetAttachParentActor()->GetActorRotation();

			lower_arm_rotation = (ActorRotation - parentRotation).Euler().X;
		}

		if (upper_arm_component && upper_arm_component->GetAttachParentActor())
		{
			FRotator ActorRotation = upper_arm_component->GetActorRotation();
			FRotator parentRotation = upper_arm_component->GetAttachParentActor()->GetActorRotation();

			upper_arm_rotation = (ActorRotation - parentRotation).Euler().X;
		}

		if (wrist_component && wrist_component->GetAttachParentActor())
		{
			FRotator ActorRotation = wrist_component->GetActorRotation();
			FRotator parentRotation = wrist_component->GetAttachParentActor()->GetActorRotation();

			wrist_rotation = (ActorRotation - parentRotation).Euler().X;
		}

		if (hand_component && hand_component->GetAttachParentActor())
		{
			FRotator ActorRotation = hand_component->GetActorRotation();
			FRotator parentRotation = hand_component->GetAttachParentActor()->GetActorRotation();

			hand_rotation = (ActorRotation - parentRotation).Euler().X;
		}
	}

	CircularClamp(base_rotation, min_rotations[0], max_rotations[0]);
	CircularClamp(lower_arm_rotation, min_rotations[1], max_rotations[1]);
	CircularClamp(upper_arm_rotation, min_rotations[2], max_rotations[2]);
	CircularClamp(wrist_rotation, min_rotations[3], max_rotations[3]);
	CircularClamp(hand_rotation, min_rotations[4], max_rotations[4]);

	if (base_component && lower_arm_component && upper_arm_component && wrist_component && hand_component)
	{
		int before = NumOverlaps();
		base_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(0, 0, base_rotation)));
		lower_arm_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(lower_arm_rotation, 0, 0)));
		upper_arm_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(upper_arm_rotation, 0, 0)));
		wrist_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(wrist_rotation, 0, 0)));
		hand_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(0, hand_rotation, 0)));
		int after = NumOverlaps();

		if (after > before && check_collision)
		{
			base_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(0, 0, std::get<0>(last_rotations))));
			base_rotation = std::get<0>(last_rotations);

			lower_arm_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(std::get<1>(last_rotations), 0, 0)));
			lower_arm_rotation = std::get<1>(last_rotations);
			
			upper_arm_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(std::get<2>(last_rotations), 0, 0)));
			upper_arm_rotation = std::get<2>(last_rotations);

			wrist_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(std::get<3>(last_rotations), 0, 0)));
			wrist_rotation = std::get<3>(last_rotations);

			hand_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(0, std::get<4>(last_rotations), 0)));
			hand_rotation = std::get<4>(last_rotations);
		}
	}
	
	last_rotations = {base_rotation, lower_arm_rotation, upper_arm_rotation, wrist_rotation, hand_rotation};
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
	double wrist_servo =
		interpolate(wrist_rotation, min_rotations[3], max_rotations[3], min_servo[3], max_servo[3]);
	double hand_servo =
		interpolate(hand_rotation, min_rotations[4], max_rotations[4], min_servo[4], max_servo[4]);
	
	std::string msg = "> " + std::to_string(int(base_servo)) + " " + std::to_string(int(lower_arm_servo)) + " " +
		std::to_string(int(upper_arm_servo)) + " " + std::to_string(int(wrist_servo)) + " " + std::to_string(int(hand_servo)) + "\n";

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
	while(serial_loop_running)
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
		base_component->GetActorLocation().X,
		base_component->GetActorLocation().Y,
		lower_arm_component->GetActorLocation().Z
	};
}

bool ARobotArm::UpdateIK(FVector target)
{
	if (!base_component || !lower_arm_component || !upper_arm_component)
		return false;

	FVector relative_position = target - ArmOrigin();

	DrawDebugSphere(GetWorld(), target, 10, 10, FColor::Purple, false, -1, 1, 2);
	
	double offset = asin(end_offset * 100 * base_component->GetActorScale3D().X / FVector2d{relative_position.X, relative_position.Y}.Length());
	
	// UE Coordinate system is sus
	double plane_angle = atan2(relative_position.X, relative_position.Y) + offset + PI;

	bool must_flip = (plane_angle > max(-min_rotations[0], -max_rotations[0]) / 180 * PI || plane_angle < min(-min_rotations[0], -max_rotations[0]) / 180 * PI);

	CircularClamp(plane_angle, -min_rotations[0] / 180 * PI, -max_rotations[0] / 180 * PI, PI);
	
	if (must_flip)
		plane_angle -= 2 * offset;
	
	base_rotation = -plane_angle * 180 / PI;	
	
	FVector plane_up = {0, 0, 1};
	FVector plane_right = {sin(plane_angle), cos(plane_angle), 0};

	FVector2d plane_position = {relative_position.Dot(plane_right), relative_position.Dot(plane_up) };

	double a = lower_arm_length * 100 * lower_arm_component->GetActorScale3D().X;
	double b = upper_arm_length * 100 * upper_arm_component->GetActorScale3D().X;
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
		
	lower_arm_rotation = -(lower_arm_radians / PI * 180 - 90);
	upper_arm_rotation = -(upper_arm_radians / PI * 180 + 90);

	return must_flip;
}

void ARobotArm::TrackBall()
{
	if (!ball || !ball->tracking_path.IsValid() || !base_component || !hand_component || !wrist_component)
		return;
	
	double intersection_radius = arm_range * 100 * base_component->GetActorScale3D().X;
	if (draw_debug)
		DrawDebugSphere(GetWorld(), ArmOrigin(), intersection_radius, 100, FColor::Blue, 0, -1);
	
	auto intersections = ball->tracking_path.IntersectSphere(ArmOrigin(), intersection_radius);

	// only times in the future are valid - obviously
	remove_if(intersections.begin(), intersections.end(), [&](double p){ return p <= ball->tracking_path.t1 - ball->tracking_path.t0;});

	for (auto x : intersections)
		UE_LOG(LogTemp, Error, TEXT("%f"), x);

	UE_LOG(LogTemp, Error, TEXT("Now: %f"), ball->tracking_path.t1 - ball->tracking_path.t0);
	
	if (intersections.size() == 0)
		return;
	
	if (draw_debug)
		for (auto t : intersections)
			DrawDebugSphere(GetWorld(), ball->tracking_path(t), intersection_radius / 30, 10, FColor::Green, 0, -1, 0, 3);
	
	double target_time = intersections[0];
	
	FVector target = ball->tracking_path(target_time);
	FVector impact_velocity = {ball->tracking_path.vx, ball->tracking_path.vy, ball->tracking_path.derivative(target_time)};

	if (draw_debug)
		DrawDebugLine(GetWorld(), target, target + impact_velocity * 100, FColor::Red, false, -1, 1);
	
	FVector relative_position = target - ArmOrigin();
	
	bool fixed = false;
	double offset = asin(end_offset * 100 * base_component->GetActorScale3D().X / FVector2d{relative_position.X, relative_position.Y}.Length());
	
	double plane_angle = atan2(relative_position.X, relative_position.Y) + offset;

	again:
	
	auto base_rotator = FQuat(FVector::UpVector, -plane_angle).Rotator();
	impact_velocity = base_rotator.UnrotateVector(impact_velocity);
	relative_position = base_rotator.UnrotateVector(relative_position);
	
	FVector pitch_down = impact_velocity * (FVector::UpVector + FVector::RightVector);
	pitch_down.Normalize();
	double pitch_angle = atan2(-pitch_down.Y, pitch_down.Z);

	auto wrist_rotator = FQuat(FVector::ForwardVector, pitch_angle).Rotator();
	FVector roll_down = wrist_rotator.UnrotateVector(impact_velocity) * (FVector::UpVector + FVector::ForwardVector);
	roll_down.Normalize();
	double roll_angle = atan2(roll_down.X, roll_down.Z);

	FVector arm_offset = FVector{0, cos(pitch_angle), sin(pitch_angle)} * (hand_length * 100 * wrist_component->GetActorScale3D().X);
	
	bool flipped = UpdateIK(target + base_rotator.RotateVector(arm_offset));
	if (flipped && !fixed)
	{
		plane_angle -= 2 * offset;
		fixed = true;
		goto again;
	}
	
	wrist_rotation = (flipped ? PI-pitch_angle : pitch_angle) / PI * 180 - lower_arm_rotation - upper_arm_rotation;
	hand_rotation = roll_angle / PI * 180 + 90;
}

// Called every frame
void ARobotArm::Tick(float DeltaTime)
{
	if (update_rotations)
	{
		if (update_type == IK && ik_target)
			UpdateIK(ik_target->GetActorLocation());

		if (update_type == Ball)
			TrackBall();
		
		UpdateRotations();
	}
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void ARobotArm::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//UpdateRotations();

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

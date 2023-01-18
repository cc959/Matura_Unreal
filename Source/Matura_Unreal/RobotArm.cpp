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

	// base_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh0"));
	// lower_arm_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh1"));
	// upper_arm_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh2"));
	// wrist_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh3"));

	// SetRootComponent(base_component);
	//
	// lower_arm_component->SetupAttachment(base_component);
	// upper_arm_component->SetupAttachment(lower_arm_component);
	// wrist_component->SetupAttachment(upper_arm_component);

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

void CircularClamp(double& val, double low, double high, double step = 360)
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
		return;

	if (abs(fmod(low + step * 4, step) - fmod(val + step * 4, step)) < abs(fmod(high + step * 4, step) - fmod(val + step * 4, step)))
		val = low;
	else
		val = high;
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
	}

	CircularClamp(base_rotation, min_rotations[0], max_rotations[0]);
	CircularClamp(lower_arm_rotation, min_rotations[1], max_rotations[1]);
	CircularClamp(upper_arm_rotation, min_rotations[2], max_rotations[2]);
	CircularClamp(wrist_rotation, min_rotations[3], max_rotations[3]);

	if (base_component && lower_arm_component && upper_arm_component && wrist_component)
	{
		int before = NumOverlaps();
		base_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(0, 0, base_rotation)));
		lower_arm_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(lower_arm_rotation, 0, 0)));
		upper_arm_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(upper_arm_rotation, 0, 0)));
		wrist_component->SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(wrist_rotation, 0, 0)));
		int after = NumOverlaps();

		if (after > before && check_collision)
		{
			base_component->
			SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(0, 0, std::get<0>(last_rotations))));
			base_rotation = std::get<0>(last_rotations);

			lower_arm_component->
			SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(std::get<1>(last_rotations), 0, 0)));
			lower_arm_rotation = std::get<1>(last_rotations);


			upper_arm_component->
			SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(std::get<2>(last_rotations), 0, 0)));
			upper_arm_rotation = std::get<2>(last_rotations);

			wrist_component->
			SetActorRelativeRotation(FQuat::MakeFromEuler(FVector(std::get<3>(last_rotations), 0, 0)));
			wrist_rotation = std::get<3>(last_rotations);
		}
	}
	
	last_rotations = {base_rotation, lower_arm_rotation, upper_arm_rotation, wrist_rotation};
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

	std::string msg = "> " + std::to_string(int(base_servo)) + " " + std::to_string(int(lower_arm_servo)) + " " +
		std::to_string(int(upper_arm_servo)) + " " + std::to_string(int(wrist_servo)) + "\n";

	if (debug_serial)
	{
		FString blub = msg.c_str();
		UE_LOG(LogTemp, Display, TEXT("Message: \"%s\""), *blub);
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

void ARobotArm::UpdateIK()
{
	if (!ik_target || !base_component || !lower_arm_component || !upper_arm_component || !wrist_component)
		return;

	FVector relative_position =
	{
		ik_target->GetActorLocation().X - base_component->GetActorLocation().X,
		ik_target->GetActorLocation().Y - base_component->GetActorLocation().Y,
		ik_target->GetActorLocation().Z - lower_arm_component->GetActorLocation().Z
	};
	
	// UE Coordinate system is sus
	double plane_angle = atan2(relative_position.X, relative_position.Y) + PI;
	
	CircularClamp(plane_angle, -min_rotations[0] / 180 * PI, -max_rotations[0] / 180 * PI, PI);
	base_rotation = -plane_angle * 180 / PI;	
	
	FVector plane_up = {0, 0, 1};
	FVector plane_right = {sin(plane_angle), cos(plane_angle), 0};

	FVector2d target_position = {relative_position.Dot(plane_right), relative_position.Dot(plane_up) };



	double a = lower_arm_length * 100 * lower_arm_component->GetActorScale3D().X;
	double b = upper_arm_length * 100 * upper_arm_component->GetActorScale3D().X;
	double c = target_position.Length();

	double lower_arm_radians;
	double target_angle = atan2(target_position.Y, target_position.X);
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
	
	double upper_arm_radians = atan2(target_position.Y - lower_arm_end.Y, target_position.X - lower_arm_end.X) - lower_arm_radians;
	CircularClamp(upper_arm_radians, (-min_rotations[2] - 90) / 180 * PI, (-max_rotations[2] - 90) / 180 * PI, 2 * PI);
		
	lower_arm_rotation = -(lower_arm_radians / PI * 180 - 90);
	upper_arm_rotation = -(upper_arm_radians / PI * 180 + 90);
}


// Called every frame
void ARobotArm::Tick(float DeltaTime)
{
	if (auto_update_rotations)
	{
		if (update_type == IK)
			UpdateIK();
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

// Fill out your copyright notice in the Description page of Project Settings.

#include "RobotArm.h"

#include <fcntl.h>	 // Contains file controls like O_RDWR
#include <errno.h>	 // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>	 // write(), read(), close()
#include <string>

// Sets default values
ARobotArm::ARobotArm()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	base_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh0"));
	lower_arm_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh1"));
	upper_arm_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh2"));
	wrist_component = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh3"));

	SetRootComponent(base_component);

	lower_arm_component->SetupAttachment(base_component);
	upper_arm_component->SetupAttachment(lower_arm_component);
	wrist_component->SetupAttachment(upper_arm_component);
}

// Called when the game starts or when spawned
void ARobotArm::BeginPlay()
{
	base_component->SetMaterial(0, UMaterialInstanceDynamic::Create(base_component->GetMaterial(0), this));

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

	tty.c_cflag &= ~PARENB;	 // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB;	 // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag &= ~CSIZE;	 // Clear all the size bits, then use one of the statements below
	tty.c_cflag |= CS8;		 // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_lflag &= ~ICANON;
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ECHO;	// Disable echo
	tty.c_lflag &= ~ECHOE;	// Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo

	tty.c_lflag &= ~ISIG;					// Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
					 ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)

	tty.c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;	  // return after reading at least VMIN bytes

	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);

	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
	{
		FString errorText(strerror(errno));
		UE_LOG(LogTemp, Display, TEXT("Error from tcsetattr: %s"), *errorText);
	}
}


void ARobotArm::UpdateRotations()
{
	auto current_base_rotation = base_component->GetRelativeRotation().Euler();
	auto current_lower_arm_rotation = lower_arm_component->GetRelativeRotation().Euler();
	auto current_upper_arm_rotation = upper_arm_component->GetRelativeRotation().Euler();
	auto current_wrist_rotation = wrist_component->GetRelativeRotation().Euler();

	current_base_rotation.Z = base_rotation;
	current_lower_arm_rotation.X = lower_arm_rotation;
	current_upper_arm_rotation.X = upper_arm_rotation;
	current_wrist_rotation.X = wrist_rotation;

	base_component->SetRelativeRotation(FQuat::MakeFromEuler(current_base_rotation));
	lower_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(current_lower_arm_rotation));
	upper_arm_component->SetRelativeRotation(FQuat::MakeFromEuler(current_upper_arm_rotation));
	wrist_component->SetRelativeRotation(FQuat::MakeFromEuler(current_wrist_rotation));
}

void ARobotArm::SendRotations()
{
	if (serial_port < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Serial port is not open, can't send data!"));
		return;
	}
	// check how many bytes can be read
	// int bytes_available;
	// ioctl(serial_port, FIONREAD, &bytes_available);
	//
	// if (bytes_available > 0)
	// {
	// 	vector<char> read_buf(bytes_available, 0);
	//
	// 	// Read bytes. The behaviour of read() (e.g. does it block?,
	// 	// how long does it block for?) depends on the configuration
	// 	// settings above, specifically VMIN and VTIME
	// 	int bytes_read = read(serial_port, &read_buf[0], sizeof(char) * read_buf.size());
	//	}

	std::string msg = "> "+  std::to_string(int(base_rotation)) + " " + std::to_string(int(lower_arm_rotation)) + " " + std::to_string(int(upper_arm_rotation)) + " " + std::to_string(int(wrist_rotation)) +"\n";
	write(serial_port, &msg[0], sizeof(msg[0]) * msg.size());

	tcdrain(serial_port);
	tcflush(serial_port, TCIOFLUSH);
}


// Called every frame
void ARobotArm::Tick(float DeltaTime)
{
	UpdateRotations();
	
	if (!visual_only)
		SendRotations();

	Super::Tick(DeltaTime);
}

#ifdef WITH_EDITOR
void ARobotArm::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	UpdateRotations();
	SendRotations();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
// Fill out your copyright notice in the Description page of Project Settings.

#include "SerialActor.h"

#include <fcntl.h>	 // Contains file controls like O_RDWR
#include <errno.h>	 // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>	 // write(), read(), close()
#include <sys/ioctl.h>

#include <string>
#include <sstream>
#include <vector>

using namespace std;

// Sets default values
ASerialActor::ASerialActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	serial_port = 0;

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static mesh"));
	mesh->SetSimulatePhysics(true);
	mesh->SetEnableGravity(true);
	SetRootComponent(mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));

	if (CubeMesh.Object)
	{
		mesh->SetStaticMesh(CubeMesh.Object);
	}
}

// Called when the game starts or when spawned
void ASerialActor::BeginPlay()
{

	// code sourced from https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
	string port_path(TCHAR_TO_UTF8(*port));
	serial_port = open(port_path.c_str(), O_RDWR);

	if (serial_port < 0)
	{
		FString errorText(strerror(errno));
		UE_LOG(LogTemp, Error, TEXT("Error from open: %s"), *errorText);
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

	Super::BeginPlay();
}

// Called every frame
void ASerialActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// check how many bytes can be read
	// int bytes_available;
	// ioctl(serial_port, FIONREAD, &bytes_available);
	//
	// UE_LOG(LogTemp, Display, TEXT("read %d bytes"), bytes_available);
	//
	// if (bytes_available > 0)
	// {
	//
	// 	vector<char> read_buf(bytes_available, 0);
	//
	// 	// Read bytes. The behaviour of read() (e.g. does it block?,
	// 	// how long does it block for?) depends on the configuration
	// 	// settings above, specifically VMIN and VTIME
	// 	int bytes_read = read(serial_port, &read_buf[0], sizeof(char) * read_buf.size());
	//
	// 	int start = 0;
	// 	while (read_buf[start] != '\n')
	// 		start++;
	//
	// 	stringstream ss;
	// 	ss << &read_buf[0];
	//
	// 	ss >> pot_value;
	//
	// 	auto transform = GetTransform();
	//
	// 	auto position = transform.GetTranslation();
	// 	position.X = pot_value;
	//
	// 	transform.SetTranslation(position);
	// 	SetActorTransform(transform);
	// }

	string msg = "> " + to_string(arm_position) + " " + to_string(base_position) + "\n";
	write(serial_port, &msg[0], sizeof(msg[0]) * msg.size());

	tcdrain(serial_port);
	tcflush(serial_port, TCIOFLUSH);
}

void ASerialActor::BeginDestroy()
{
	Super::BeginDestroy();

	close(serial_port);
}

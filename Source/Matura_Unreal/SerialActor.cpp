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

	Super::BeginPlay();
}

// Called every frame
void ASerialActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASerialActor::BeginDestroy()
{
	Super::BeginDestroy();

	close(serial_port);
}

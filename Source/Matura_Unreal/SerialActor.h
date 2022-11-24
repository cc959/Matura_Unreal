// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SerialActor.generated.h"

#include <fcntl.h>	 // Contains file controls like O_RDWR
#include <errno.h>	 // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>	 // write(), read(), close()

UCLASS()
class MATURA_UNREAL_API ASerialActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASerialActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay()

		override;

public:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SerialInfo)
	FString port;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SerialInfo)
	int serial_port;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SerialInfo)
	int pot_value;

	UPROPERTY(EditAnywhere)
	bool led_active;

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void BeginDestroy() override;
};

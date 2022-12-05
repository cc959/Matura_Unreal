// Fill out your copyright notice in the Description page of Project Settings.


#include "FlyCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MyUserWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

// Sets default values
AFlyCharacter::AFlyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(ADefaultPawn::CollisionComponentName);
	CollisionComponent->InitSphereRadius(35.0f);
	CollisionComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	CollisionComponent->CanCharacterStepUpOn = ECB_No;
	CollisionComponent->SetShouldUpdatePhysicsVolume(true);
	CollisionComponent->SetCanEverAffectNavigation(false);
	CollisionComponent->bDynamicObstacle = true;

	RootComponent = CollisionComponent;

	MovementComponent = CreateDefaultSubobject<UPawnMovementComponent, UFloatingPawnMovement>(ADefaultPawn::MovementComponentName);
	MovementComponent->UpdatedComponent = CollisionComponent;

	ConstructorHelpers::FClassFinder<UMyUserWidget> hud(TEXT("/Game/CameraSelectUI"));
	if (hud.Class)
		hud_class = hud.Class;
}

void AFlyCharacter::InitGUI()
{
	if (hud_class)
	{
		APlayerController* player_controller = Cast<APlayerController>(Controller);
		if (player_controller)
		{
			hud_instance = CreateWidget<UMyUserWidget>(player_controller, hud_class);
			hud_instance->AddToViewport();
		} else {
			UE_LOG(LogTemp, Warning, TEXT("Is not controlled by a player, can't add UI: Fly Character"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UI Class is null: Tracking Camera"));
	}
}

void AFlyCharacter::DestroyGUI()
{
	if (hud_instance)
		DestructItem(hud_instance);
}

// Called when the game starts or when spawned
void AFlyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitGUI();
}

// Called every frame
void AFlyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AFlyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_MoveForward", EKeys::W, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_MoveForward", EKeys::S, -1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_MoveRight", EKeys::A, -1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_MoveRight", EKeys::D, 1.f));

	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_MoveUp", EKeys::LeftShift, -1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_MoveUp", EKeys::SpaceBar, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_MoveUp", EKeys::E, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_MoveUp", EKeys::Q, -1.f));
	
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_Turn", EKeys::MouseX, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("FlyCharacter_LookUp", EKeys::MouseY, -1.f));

	PlayerInputComponent->BindAxis("FlyCharacter_MoveForward", this, &AFlyCharacter::MoveForward);
	PlayerInputComponent->BindAxis("FlyCharacter_MoveRight", this, &AFlyCharacter::MoveRight);
	PlayerInputComponent->BindAxis("FlyCharacter_MoveUp", this, &AFlyCharacter::MoveUp_World);
	PlayerInputComponent->BindAxis("FlyCharacter_Turn", this, &AFlyCharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis("FlyCharacter_LookUp", this, &AFlyCharacter::AddControllerPitchInput);
}


void AFlyCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		if (Controller)
		{
			FRotator const ControlSpaceRot = Controller->GetControlRotation();

			// transform to world space and add it
			AddMovementInput( FRotationMatrix(ControlSpaceRot).GetScaledAxis( EAxis::Y ), Val );
		}
	}
}

void AFlyCharacter::MoveForward(float Val)
{
	if (Val != 0.f)
	{
		if (Controller)
		{
			FRotator const ControlSpaceRot = Controller->GetControlRotation();

			// transform to world space and add it
			AddMovementInput( FRotationMatrix(ControlSpaceRot).GetScaledAxis( EAxis::X ), Val );
		}
	}
}

void AFlyCharacter::MoveUp_World(float Val)
{
	if (Val != 0.f)
	{
		AddMovementInput(FVector::UpVector, Val);
	}
}

UPawnMovementComponent* AFlyCharacter::GetMovementComponent() const
{
	return MovementComponent;
}



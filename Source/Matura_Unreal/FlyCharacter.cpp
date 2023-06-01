// Fill out your copyright notice in the Description page of Project Settings.


#include "FlyCharacter.h"

#include <EngineUtils.h>
#include <Camera/CameraActor.h>

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
			hud_instance->GetWidgetFromName("camera_preview")->SetVisibility(ESlateVisibility::Hidden);
			hud_instance->GetWidgetFromName("slide")->SetVisibility(ESlateVisibility::Hidden);
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
	TActorIterator<ACameraActor> ActorItr(GetWorld());

	if (ActorItr)
	{
		auto target_position = ActorItr->GetActorLocation();
		auto target_rotation = ActorItr->GetActorRotation().Quaternion();

		auto current_position = GetActorLocation();
		auto current_rotation = Controller->GetControlRotation().Quaternion();

		if ((target_position - last_target_position).Length() > 1 || target_rotation.AngularDistance(last_target_rotation) > PI / 180)
		{
			locked = true;
		}

		if (GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->WasInputKeyJustPressed(FKey("R")))
		{
			locked = true;
		}
		
		auto new_position = current_position * (1-0.1) + (target_position - FVector(0, 0, 64)) * 0.1;
		auto new_rotation = UE::Math::TQuat<double>::Slerp(current_rotation, target_rotation, 0.1);

		if (teleport)
		{
			new_position = target_position - FVector(0, 0, 64);
			new_rotation = target_rotation;
			teleport = false;
		}

		if (locked)
		{
			SetActorLocation(new_position);
			Controller->SetControlRotation(new_rotation.Rotator());

			last_target_position = target_position;
			last_target_rotation = target_rotation;
		}

		if ((current_position - (target_position - FVector(0, 0, 64))).Length() < 1 && current_rotation.AngularDistance(target_rotation) < PI / 180)
		{
			locked = false;
		}
		
	} else
	{
		last_target_position = FVector(-1e20);
		last_target_rotation = FQuat(-1e20);
	}
	
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



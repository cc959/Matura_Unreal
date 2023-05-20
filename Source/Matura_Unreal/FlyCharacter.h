// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "InputMappingQuery.h"
#include "GameFramework/Character.h"
#include "GameFramework/DefaultPawn.h"
#include "FlyCharacter.generated.h"

UCLASS()
class MATURA_UNREAL_API AFlyCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFlyCharacter();

	bool teleport = false;
	bool locked = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FVector last_target_position;
	FQuat last_target_rotation;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;

	void InitGUI();
	void DestroyGUI();
	
	/**
	 * Input callback to move forward in local space (or backward if Val is negative).
	 * @param Val Amount of movement in the forward direction (or backward if negative).
	 * @see APawn::AddMovementInput()
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void MoveForward(float Val);

	/**
	 * Input callback to strafe right in local space (or left if Val is negative).
	 * @param Val Amount of movement in the right direction (or left if negative).
	 * @see APawn::AddMovementInput()
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void MoveRight(float Val);

	/**
	 * Input callback to move up in world space (or down if Val is negative).
	 * @param Val Amount of movement in the world up direction (or down if negative).
	 * @see APawn::AddMovementInput()
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void MoveUp_World(float Val);

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMyUserWidget> hud_class;

	UPROPERTY()
	class UMyUserWidget *hud_instance;
	
protected:
	UPROPERTY(Category = Pawn, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPawnMovementComponent> MovementComponent;

private:
	UPROPERTY(Category = Pawn, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;
	
};

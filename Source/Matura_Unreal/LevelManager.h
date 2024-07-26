// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <IntVectorTypes.h>
#include <vector>
#include <Engine/StreamableManager.h>

#include "CameraControl.h"
#include "GameFramework/Actor.h"
#include "Engine/ObjectLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LevelManager.generated.h"

UENUM()
enum FoliageMode
{
	Adaptive = 0,
	AlwaysEnabled = 1,
	AlwaysDisabled = 2,
};

UENUM()
enum TransitionType
{
	Instant = 0,
	Fade = 1,
	Slide = 2
};

UCLASS()
class MATURA_UNREAL_API ALevelManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALevelManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = Game)
	void ApplyFoliageVisibility();
	
	bool key_pressed = false;

	FName current_level;
	bool foliage_visible = false;

	UFUNCTION(BlueprintCallable, Category = Game)
	void LoadCurrentLevel();

	UFUNCTION(BlueprintCallable, Category = Game)

	UTexture2D* LoadSlide(FName name);
	void ApplyViewportSize(int slide_width, int slide_height);
	bool UpdateWidgets();
	void UpdateSlideTexture(UTexture* slide_texture);

	UTexture* old_slide_texture = nullptr;

	double normal_target = 0;
	double old_target = -1;

	double normal_opacity_target = 0;
	double old_opacity_target = 0;
	
	bool switch_direction = false;
	bool instant_transition = false;
	bool fade_transition = false;

	ACameraControl* camera_control = nullptr;

	FName level_to_unload = "";
public:

	UObjectLibrary* object_library;
	FStreamableManager manager;
	TSharedPtr<FStreamableHandle> blubpointer;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere)
	TEnumAsByte<FoliageMode> foliage_mode;
	
	UPROPERTY(EditAnywhere, Category=Transitions)
	double transition_time = 0.2;

	UPROPERTY(EditAnywhere, Category=Transitions)
	TEnumAsByte<TransitionType> normal = Instant;
	
	UPROPERTY(EditAnywhere, Category=Transitions)
	TEnumAsByte<TransitionType> subslide = Fade;

	UPROPERTY(EditAnywhere, Category=Transitions)
	TEnumAsByte<TransitionType> scene = Slide;

	UPROPERTY(EditAnywhere, Category=Transitions)
	bool inverted_controls = false;
	
	UPROPERTY(EditAnywhere)
	TArray<FName> sublevels;

	

	FVector2d viewport_size = {};

	int public_level = 0;
	
	int level = 0;
	int level_loaded = -1;
	bool foliage_level_loaded = false;

	
};

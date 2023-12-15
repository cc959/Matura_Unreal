// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelManager.h"

#include <EngineUtils.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <Components/CanvasPanelSlot.h>
#include <Kismet/GameplayStatics.h>

#include "Ball.h"
#include "CameraControl.h"
#include "FlyCharacter.h"
#include "MyBlueprintFunctionLibrary.h"
#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"
#include "Runtime/Engine/Classes/Engine/RendererSettings.h"

#include <chrono>
#include <thread>

#include "GlobalIncludes.h"

// Sets default values
ALevelManager::ALevelManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevelManager::BeginPlay()
{
	if (!object_library)
	{
		object_library = UObjectLibrary::CreateLibrary(UTexture2D::StaticClass(), false, GIsEditor);
		object_library->AddToRoot();
	}
	object_library->bRecursivePaths = true;
	object_library->LoadAssetDataFromPath(TEXT("/Game/Slides/"));
	object_library->LoadAssetsFromAssetData();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> ObjectList;

	AssetRegistryModule.Get().GetAllAssets(ObjectList);

	for (FAssetData& AssetData : ObjectList)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *AssetData.GetSoftObjectPath().ToString());
		//LogWarning(TEXT("%s"), *AssetData.GetSoftObjectPath().ToString());
	}

	old_slide_texture = nullptr;

	UpdateSlideTexture(nullptr);

	camera_control = nullptr;
	level_to_unload = "";

	Super::BeginPlay();
}

void ALevelManager::ApplyFoliageVisibility()
{
	if (foliage_mode == Adaptive)
	{
		if (current_level == "")
			foliage_visible = false;
		else
		{
			TActorIterator<ABall> ActorItr(GetWorld());

			if (ActorItr)
				foliage_visible = false;
			else
				foliage_visible = true;
		}
	}
	else if (foliage_mode == AlwaysEnabled)
	{
		foliage_visible = true;
	}
	else if (foliage_mode == AlwaysDisabled)
	{
		foliage_visible = false;
	}

	if (foliage_visible != foliage_level_loaded)
	{
		if (foliage_visible)
		{
			LogDisplay(TEXT("Foliage Level Loaded %d"), foliage_level_loaded);
			UGameplayStatics::LoadStreamLevel(this, "FoliageLevel", true, true, FLatentActionInfo());
		}
		else
		{
			LogDisplay(TEXT("Foliage Level Unloaded %d"), foliage_level_loaded);
			UGameplayStatics::UnloadStreamLevel(this, "FoliageLevel", FLatentActionInfo(), true);
		}
	}
	foliage_level_loaded = foliage_visible;

	if (!sublevels[level].ToString().ToLower().StartsWith("slide_"))
	{
		UpdateSlideTexture(nullptr);
	}
}

void ALevelManager::LoadCurrentLevel()
{
	LogDisplay(TEXT("Unpaused game"));

	int old_level = level + (switch_direction ? 1 : -1);

	if (old_level >= 0 && old_level < sublevels.Num())
	{

		auto get_type = [](FString name)
		{
			if (name.Len() == 0)
				return 2;
			if (name.ToLower().StartsWith("slide_"))
			{
				if (name[0] == 's')
					return 1;
				else
					return 0;
			} else
			{
				return 2;
			}
		};
		
		int old_type = get_type(sublevels[old_level].ToString());
		int new_type = get_type(sublevels[level].ToString());

		if (switch_direction)
			swap(old_type, new_type);

		TransitionType transition_matrix[3][3] = {
			{normal, subslide, scene},
			{normal, subslide, scene},
			{scene, scene, scene},
		};

		TransitionType to_use = transition_matrix[old_type][new_type];
		
		switch (to_use)
		{
		case Fade:
			fade_transition = true;
			instant_transition = false;
			break;
		case Instant:
			fade_transition = false;
			instant_transition = true;
			break;
		case Slide:
			fade_transition = false;
			instant_transition = false;
			break;
		}
	}
	
	if (sublevels[level].ToString().ToLower().StartsWith("slide_"))
	{
		auto slide_texture = LoadSlide(sublevels[level]);
		if (slide_texture)
		{
			UpdateSlideTexture(slide_texture);
			LogDisplay(TEXT("Now displaying slide %s"), *sublevels[level].ToString());
		}
		else
		{
			LogError(TEXT("Could not load slide texture of slide %s"), *sublevels[level].ToString());
		}
		current_level = "";
	}
	else
	{
		LogDisplay(TEXT("Loading level number %d: %s, current level: %s"), level, *sublevels[level].ToString(),
		       *current_level.ToString());

		FLatentActionInfo info;
		info.CallbackTarget = this;
		info.ExecutionFunction = "ApplyFoliageVisibility";
		info.UUID = 1;
		info.Linkage = 0;

		UGameplayStatics::LoadStreamLevel(this, sublevels[level], true, true, info);
		current_level = sublevels[level];
	}
}

UTexture2D* ALevelManager::LoadSlide(FName name)
{
	auto name_string = name.ToString();
	if (name_string.StartsWith("slide_"))
	{
		name_string[0] = 'S';
	}
	TArray<FAssetData> AssetDatas;
	object_library->GetAssetDataList(AssetDatas);

	TArray<FSoftObjectPath> AssetPaths;
	for (FAssetData& AssetData : AssetDatas)
	{
		if (AssetData.AssetName.ToString() == name_string)
		{
			AssetPaths.Push(AssetData.GetSoftObjectPath());
			break;
		}
	}

	blubpointer = manager.RequestSyncLoad(AssetPaths);

	if (blubpointer.IsValid() && blubpointer->IsActive())
	{
		while (!blubpointer->HasLoadCompleted())
			usleep(10);
		return Cast<UTexture2D>(blubpointer->GetLoadedAsset());
	}
	return nullptr;
}

void ALevelManager::ApplyViewportSize(int slide_width, int slide_height)
{
	if (!camera_control)
		camera_control = Cast<ACameraControl>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

	if (camera_control && Cast<AFlyCharacter>(camera_control->GetPawn()))
	{
		auto fly = Cast<AFlyCharacter>(camera_control->GetPawn());
		if (fly->hud_instance)
		{
			for (auto widget : std::vector{fly->hud_instance->GetWidgetFromName("slide"), fly->hud_instance->GetWidgetFromName("old_slide")})
				if (UImage* slide = Cast<UImage>(widget))
				{
					if (auto slot = Cast<UCanvasPanelSlot>(slide->Slot))
					{
						double slide_aspect = slide_width / double(slide_height);
						double viewport_aspect = viewport_size.X / viewport_size.Y;

						FVector2d slide_size{double(slide_width), double(slide_height)};

						if (slide_aspect >= viewport_aspect)
						{
							double ratio = (viewport_size.X / slide_size.X);
							slot->SetSize(slide_size * ratio);
							slot->SetPosition({0, (viewport_size.Y - (slide_size.Y * ratio)) / 2});
						}
						else
						{
							double ratio = (viewport_size.Y / slide_size.Y);
							slot->SetSize(slide_size * ratio);
							slot->SetPosition({(viewport_size.X - (slide_size.X * ratio)) / 2, 0});
						}
					}
				}
		}
	}
}

bool ALevelManager::UpdateWidgets()
{
	if (!camera_control)
		camera_control = Cast<ACameraControl>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

	bool transition_is_done = true;

	if (camera_control && Cast<AFlyCharacter>(camera_control->GetPawn()))
	{
		auto fly = Cast<AFlyCharacter>(camera_control->GetPawn());
		if (fly->hud_instance)
		{
			for (auto widget : std::vector{fly->hud_instance->GetWidgetFromName("slide"), fly->hud_instance->GetWidgetFromName("slide_bg")})
				if (UImage* slide = Cast<UImage>(widget))
				{
					auto render_transform = slide->GetRenderTransform();
					double offset = slide->GetRenderTransform().Translation.Y - viewport_size.Y * normal_target;
					if (!instant_transition)
						offset = std::clamp(offset, -viewport_size.Y * GetWorld()->DeltaTimeSeconds / transition_time,
						                    viewport_size.Y * GetWorld()->DeltaTimeSeconds / transition_time);
					render_transform.Translation.Y -= offset;
					slide->SetRenderTransform(render_transform);

					double opacity_offset = slide->GetRenderOpacity() - normal_opacity_target;
					if (!instant_transition)
						opacity_offset = std::clamp(opacity_offset, -GetWorld()->DeltaTimeSeconds / transition_time,
						                            GetWorld()->DeltaTimeSeconds / transition_time);
					slide->SetRenderOpacity(slide->GetRenderOpacity() - opacity_offset);

					transition_is_done &= abs(opacity_offset) < 1e-5;
					transition_is_done &= abs(offset) < 1e-5;
				}

			for (auto widget : std::vector{fly->hud_instance->GetWidgetFromName("old_slide"), fly->hud_instance->GetWidgetFromName("old_slide_bg")})
				if (UImage* slide = Cast<UImage>(widget))
				{
					auto render_transform = slide->GetRenderTransform();
					double offset = slide->GetRenderTransform().Translation.Y - viewport_size.Y * old_target;
					if (!instant_transition)
						offset = std::clamp(offset, -viewport_size.Y * GetWorld()->DeltaTimeSeconds / transition_time,
						                    viewport_size.Y * GetWorld()->DeltaTimeSeconds / transition_time);
					render_transform.Translation.Y -= offset;
					slide->SetRenderTransform(render_transform);

					double opacity_offset = slide->GetRenderOpacity() - old_opacity_target;
					if (!instant_transition)
						opacity_offset = std::clamp(opacity_offset, -GetWorld()->DeltaTimeSeconds / transition_time,
						                            GetWorld()->DeltaTimeSeconds / transition_time);
					slide->SetRenderOpacity(slide->GetRenderOpacity() - opacity_offset);

					transition_is_done &= abs(opacity_offset) < 1e-5;
					transition_is_done &= abs(offset) < 1e-5;
				}
		}
	}

	if (instant_transition)
		instant_transition = false;

	return transition_is_done;
}

void ALevelManager::UpdateSlideTexture(UTexture* slide_texture)
{
	bool show_new = bool(slide_texture);
	bool show_old = bool(old_slide_texture);

	if (switch_direction && !fade_transition)
		swap(show_new, show_old);


	if (fade_transition)
	{
		normal_target = old_target = 0;

		normal_opacity_target = show_new;
		old_opacity_target = show_new;
	}
	else
	{
		normal_opacity_target = show_new;
		old_opacity_target = show_old;

		normal_target = !switch_direction ? 0 : 1;
		old_target = !switch_direction ? -1 : 0;
	}

	if (!slide_texture)
	{
		LogError(TEXT("Slide texture is null!"));
	}

	if (!camera_control)
		camera_control = Cast<ACameraControl>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

	if (camera_control && Cast<AFlyCharacter>(camera_control->GetPawn()))
	{
		auto fly = Cast<AFlyCharacter>(camera_control->GetPawn());
		if (fly->hud_instance)
		{
			if (UImage* slide = Cast<UImage>(fly->hud_instance->GetWidgetFromName("slide")))
			{
				if (show_new)
				{
					auto brush = slide->GetBrush();
					brush.SetResourceObject(!switch_direction || fade_transition ? slide_texture : old_slide_texture);
					slide->SetBrush(brush);
				}
				slide->SetVisibility(ESlateVisibility::Visible);
				slide->SetRenderOpacity(!fade_transition ? show_new : false);

				auto render_transform = slide->GetRenderTransform();
				if (fade_transition)
					render_transform.Translation.Y = 0;
				else
					render_transform.Translation.Y = !switch_direction ? viewport_size.Y : 0;
				slide->SetRenderTransform(render_transform);
			}

			if (UImage* old_slide = Cast<UImage>(fly->hud_instance->GetWidgetFromName("old_slide")))
			{
				if (show_old)
				{
					auto brush = old_slide->GetBrush();
					brush.SetResourceObject(!switch_direction || fade_transition ? old_slide_texture : slide_texture);
					old_slide->SetBrush(brush);
				}
				old_slide->SetVisibility(ESlateVisibility::Visible);
				old_slide->SetRenderOpacity(!fade_transition ? show_old : show_new);

				auto render_transform = old_slide->GetRenderTransform();
				if (fade_transition)
					render_transform.Translation.Y = 0;
				else
					render_transform.Translation.Y = !switch_direction ? 0 : -viewport_size.Y;
				old_slide->SetRenderTransform(render_transform);
			}

			if (UImage* slide_bg = Cast<UImage>(fly->hud_instance->GetWidgetFromName("slide_bg")))
			{
				slide_bg->SetVisibility(ESlateVisibility::Visible);
				slide_bg->SetRenderOpacity(!fade_transition ? show_new : false);

				auto render_transform = slide_bg->GetRenderTransform();
				if (fade_transition)
					render_transform.Translation.Y = 0;
				else
					render_transform.Translation.Y = !switch_direction ? viewport_size.Y : 0;
				slide_bg->SetRenderTransform(render_transform);
			}

			if (UImage* old_slide_bg = Cast<UImage>(fly->hud_instance->GetWidgetFromName("old_slide_bg")))
			{
				old_slide_bg->SetVisibility(ESlateVisibility::Visible);
				old_slide_bg->SetRenderOpacity(!fade_transition ? show_old : show_new);

				auto render_transform = old_slide_bg->GetRenderTransform();
				if (fade_transition)
					render_transform.Translation.Y = 0;
				else
					render_transform.Translation.Y = !switch_direction ? 0 : -viewport_size.Y;
				old_slide_bg->SetRenderTransform(render_transform);
			}
		}
	}

	old_slide_texture = slide_texture;
	if (auto old_slide_texture_2d = Cast<UTexture2D>(old_slide_texture))
		ApplyViewportSize(old_slide_texture_2d->GetSizeX(), old_slide_texture_2d->GetSizeY());
}

// Called every frame
void ALevelManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	bool transition_is_done = UpdateWidgets();

	if (!transition_is_done)
	{
		return;
	}
	
	if (level_to_unload != "")
	{
		UGameplayStatics::UnloadStreamLevel(this, level_to_unload, FLatentActionInfo(), true);
		level_to_unload = "";
		ApplyFoliageVisibility();
		return;
	}

	TActorIterator<ASequencePlayer> player(GetWorld());
	
	if (public_level != level)
	{
		level = public_level;
	}
	else
	{
		if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::PageDown))
		{
			if (key_pressed == false)
			{
				level++;
				public_level = level;
			}
			key_pressed = true;
		}
		else if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::PageUp))
		{
			if (key_pressed == false)
			{
				level--;
				public_level = level;
			}
			key_pressed = true;
		} else if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::Right))
		{
			if (key_pressed == false)
			{
				if (!player || player->MoveForward())
				{
					level++;
					public_level = level;
				}
			}
			key_pressed = true;
		} else if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::Left))
		{
			if (key_pressed == false)
			{
				if (!player || player->MoveBackward())
				{
					level--;
					public_level = level;
				}
			}
			key_pressed = true;
		}
		else
		{
			key_pressed = false;
		}
	}

	if (level >= 0 && level < sublevels.Num() && !sublevels[level].IsNone())
	{
		FVector2d new_viewport_size;
		GetWorld()->GetGameViewport()->GetViewportSize(new_viewport_size);
		float dpi = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(
			FIntPoint(new_viewport_size.X, new_viewport_size.Y));
		new_viewport_size /= dpi;
		if (new_viewport_size != viewport_size && sublevels[level].ToString().ToLower().StartsWith("slide_"))
		{
			viewport_size = new_viewport_size;
			if (auto old_slide_texture_2d = Cast<UTexture2D>(old_slide_texture))
				ApplyViewportSize(old_slide_texture_2d->GetSizeX(), old_slide_texture_2d->GetSizeY());
		}

		if (level != level_loaded)
		{
			switch_direction = level < level_loaded;
			level_loaded = level;

			if (current_level != "")
			{
				if (sublevels[level].ToString().ToLower().StartsWith("slide_"))
				{
					level_to_unload = current_level;
					LoadCurrentLevel();
				}
				else
				{
					UGameplayStatics::UnloadStreamLevel(this, current_level, FLatentActionInfo(), true);
					LoadCurrentLevel();
				}
			}
			else
			{
				LoadCurrentLevel();
				TActorIterator<AFlyCharacter> ActorItr(GetWorld());

				if (ActorItr)
					ActorItr->teleport = true;
			}
		}
	}
	else
	{
		level = public_level = level_loaded;
		LogError(TEXT("Error loading level number %d"), level);
	}
}

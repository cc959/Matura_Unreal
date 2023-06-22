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
		//UE_LOG(LogTemp, Warning, TEXT("%s"), *AssetData.GetSoftObjectPath().ToString());
	}

	HideSlides();

	Super::BeginPlay();
}

void ALevelManager::ApplyFoliageVisibility()
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

	if (foliage_visible != foliage_level_loaded)
	{
		if (foliage_visible)
		{
			UE_LOG(LogTemp, Display, TEXT("Foliage Level Loaded %d"), foliage_level_loaded);
			UGameplayStatics::LoadStreamLevel(this, "FoliageLevel", true, true, FLatentActionInfo());
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("Foliage Level Unloaded %d"), foliage_level_loaded);
			UGameplayStatics::UnloadStreamLevel(this, "FoliageLevel", FLatentActionInfo(), true);
		}
	}
	foliage_level_loaded = foliage_visible;

	if (!sublevels[level].ToString().StartsWith("Slide_"))
	{
		HideSlides();
	}
}

void ALevelManager::LoadCurrentLevel()
{
	UGameplayStatics::SetGamePaused(this, false);
	UE_LOG(LogTemp, Display, TEXT("Unpaused game"));

	if (sublevels[level].ToString().StartsWith("Slide_"))
	{
		auto slide_texture = LoadSlide(sublevels[level]);
		if (slide_texture)
		{
			SetSlideTexture(slide_texture, slide_texture->GetSizeX(), slide_texture->GetSizeY());
			UE_LOG(LogTemp, Display, TEXT("Now displaying slide %s"), *sublevels[level].ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Could not load slide texture of slide %s"), *sublevels[level].ToString());
		}
		current_level = "";
		ApplyFoliageVisibility();
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Loading level number %d: %s, current level: %s"), level, *sublevels[level].ToString(),
		       *current_level.ToString());

		FLatentActionInfo info;
		info.CallbackTarget = this;
		info.ExecutionFunction = "ApplyFoliageVisibility";
		info.UUID = 1;
		info.Linkage = 0;

		UGameplayStatics::LoadStreamLevel(this, sublevels[level], true, true, info);
		current_level = sublevels[level];
	}

	// UGameplayStatics::SetGamePaused(this, false);
}

void ALevelManager::FinishLoading()
{
	UE_LOG(LogTemp, Display, TEXT("Done unloading"));
	finished_loading = true;
}

void ALevelManager::HideSlides()
{
	auto camera_control = Cast<ACameraControl>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

	if (camera_control && Cast<AFlyCharacter>(camera_control->GetPawn()))
	{
		auto fly = Cast<AFlyCharacter>(camera_control->GetPawn());
		if (fly->hud_instance)
		{
			if (UImage* slide = Cast<UImage>(fly->hud_instance->GetWidgetFromName("slide")))
				slide->SetVisibility(ESlateVisibility::Hidden);

			if (UImage* slide_bg = Cast<UImage>(fly->hud_instance->GetWidgetFromName("slide_bg")))
				slide_bg->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

UTexture2D* ALevelManager::LoadSlide(FName name)
{
	TArray<FAssetData> AssetDatas;
	object_library->GetAssetDataList(AssetDatas);

	TArray<FSoftObjectPath> AssetPaths;
	for (FAssetData& AssetData : AssetDatas)
	{
		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *AssetData.GetSoftObjectPath().ToString());

		if (AssetData.AssetName == name)
		{
			AssetPaths.Push(AssetData.GetSoftObjectPath());
			break;
		}
	}


	blubpointer = manager.RequestSyncLoad(AssetPaths);

	if (blubpointer.IsValid() && blubpointer->IsActive())
	{
		while (!blubpointer->HasLoadCompleted()) usleep(10);
		return Cast<UTexture2D>(blubpointer->GetLoadedAsset());
	}
	return nullptr;
}

void ALevelManager::SetSlideTexture(UTexture* slide_texture, int width, int height)
{
	if (!slide_texture)
	{
		UE_LOG(LogTemp, Error, TEXT("Slide texture is null!"));
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Slide texture is null!"));
		return;
	}

	auto camera_control = Cast<ACameraControl>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

	if (camera_control && Cast<AFlyCharacter>(camera_control->GetPawn()))
	{
		auto fly = Cast<AFlyCharacter>(camera_control->GetPawn());
		if (fly->hud_instance)
		{
			if (UImage* slide = Cast<UImage>(fly->hud_instance->GetWidgetFromName("slide")))
			{
				if (auto slot = Cast<UCanvasPanelSlot>(slide->Slot))
				{
					double slide_aspect = width / double(height);
					double viewport_aspect = viewport_size.X / viewport_size.Y;

					FVector2d slide_size{double(width), double(height)};

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
				if (width != 0 || height != 0)
				{
					auto brush = slide->GetBrush();
					brush.SetResourceObject(slide_texture);
					slide->SetBrush(brush);
				}
				slide->SetVisibility(ESlateVisibility::Visible);
			}

			if (UImage* slide_bg = Cast<UImage>(fly->hud_instance->GetWidgetFromName("slide_bg")))
			{
				if (auto slot = Cast<UCanvasPanelSlot>(slide_bg->Slot))
				{
					slot->SetSize(viewport_size);
					slot->SetPosition({0, 0});
				}

				slide_bg->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}

// Called every frame
void ALevelManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::PageDown))
	{
		if (key_pressed == false)
		{
			level++;
		}
		key_pressed = true;
	}
	else if (GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::PageUp))
	{
		if (key_pressed == false)
		{
			level--;
		}
		key_pressed = true;
	}
	else
	{
		key_pressed = false;
	}


	if (level >= 0 && level < sublevels.Num() && !sublevels[level].IsNone())
	{
		FVector2d new_viewport_size;
		GetWorld()->GetGameViewport()->GetViewportSize(new_viewport_size);
		float dpi = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(
			FIntPoint(new_viewport_size.X, new_viewport_size.Y));
		new_viewport_size /= dpi;
		if (new_viewport_size != viewport_size && sublevels[level].ToString().StartsWith("Slide_"))
		{
			viewport_size = new_viewport_size;
			auto slide_texture = LoadSlide(sublevels[level]);
			if (slide_texture)
			{
				SetSlideTexture(slide_texture, slide_texture->GetSizeX(), slide_texture->GetSizeY());
				UE_LOG(LogTemp, Display, TEXT("Now displaying slide %s"), *sublevels[level].ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Could not load slide texture of slide %s"), *sublevels[level].ToString());
			}
		}

		if (level != level_loaded)
		{
			level_loaded = level;

			// for some reason can't load and unload a level on the same frame, or perhaps I did it incorrectly
			if (current_level != "")
			{
				UGameplayStatics::UnloadStreamLevel(this, current_level, FLatentActionInfo(), true);
				LoadCurrentLevel();
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
		level = level_loaded;
		UE_LOG(LogTemp, Error, TEXT("Error loading level number %d"), level);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelManager.h"

#include <Components/CanvasPanelSlot.h>
#include <Kismet/GameplayStatics.h>

#include "CameraControl.h"
#include "FlyCharacter.h"
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
	slide_textures.resize(sublevels.Num(), nullptr);
	Super::BeginPlay();
}

void HideSlides(ALevelManager* you)
{
	auto camera_control = Cast<ACameraControl>(UGameplayStatics::GetPlayerController(you->GetWorld(), 0));

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

void SetSlideTexture(ALevelManager* you, UTexture2D* slide_texture)
{
	auto camera_control = Cast<ACameraControl>(UGameplayStatics::GetPlayerController(you->GetWorld(), 0));

	if (camera_control && Cast<AFlyCharacter>(camera_control->GetPawn()))
	{
		auto fly = Cast<AFlyCharacter>(camera_control->GetPawn());
		if (fly->hud_instance)
		{
			if (UImage* slide = Cast<UImage>(fly->hud_instance->GetWidgetFromName("slide")))
			{
				if (auto slot = Cast<UCanvasPanelSlot>(slide->Slot))
				{
					double slide_aspect = slide_texture->GetSizeX() / double(slide_texture->GetSizeY());
					double viewport_aspect = you->viewport_size.X / you->viewport_size.Y;

					FVector2d slide_size{double(slide_texture->GetSizeX()), double(slide_texture->GetSizeY())};
					
					if (slide_aspect >= viewport_aspect)
					{
						double ratio = (you->viewport_size.X / slide_size.X);
						slot->SetSize(slide_size * ratio);
						slot->SetPosition({0, (you->viewport_size.Y - (slide_size.Y * ratio)) / 2});
					} else
					{
						double ratio = (you->viewport_size.Y / slide_size.Y);
						slot->SetSize(slide_size * ratio);
						slot->SetPosition({(you->viewport_size.X - (slide_size.X * ratio)) / 2, 0});
					}
				}

				slide->Brush.SetResourceObject(slide_texture);
				slide->SetVisibility(ESlateVisibility::Visible);
			}

			if (UImage* slide_bg = Cast<UImage>(fly->hud_instance->GetWidgetFromName("slide_bg")))
			{
				if (auto slot = Cast<UCanvasPanelSlot>(slide_bg->Slot))
				{
					slot->SetSize(you->viewport_size);
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
			SetSlideTexture(this, slide_textures[level]);
		}
		
		if (level != level_loaded)
		{
			// for some reason can't load and unload a level on the same frame, or perhaps I did it incorrectly
			if (current_level != "")
			{
				UGameplayStatics::UnloadStreamLevel(this, current_level, FLatentActionInfo(), true);
				current_level = "";
			}
			else
			{
				level_loaded = level;

				if (level + 1 < sublevels.Num() && sublevels[level + 1].ToString().StartsWith("Slide_"))
				{
					FString slide_name = sublevels[level + 1].ToString();
					std::string path = "/Game/Slides/" + std::string(TCHAR_TO_UTF8(*slide_name)) + "." + std::string(TCHAR_TO_UTF8(*slide_name));

					slide_textures[level + 1] = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), this, *FString(path.c_str())));
				}

				if (level > 0 && sublevels[level - 1].ToString().StartsWith("Slide_"))
				{
					FString slide_name = sublevels[level - 1].ToString();
					std::string path = "/Game/Slides/" + std::string(TCHAR_TO_UTF8(*slide_name)) + "." + std::string(TCHAR_TO_UTF8(*slide_name));

					slide_textures[level - 1] = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), this, *FString(path.c_str())));
				}

				if (sublevels[level].ToString().StartsWith("Slide_"))
				{
					if (slide_textures[level] == nullptr)
					{
						FString slide_name = sublevels[level].ToString();
						std::string path = "/Game/Slides/" + std::string(TCHAR_TO_UTF8(*slide_name)) + "." + std::string(TCHAR_TO_UTF8(*slide_name));

						slide_textures[level] = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), this, *FString(path.c_str())));
					}

					SetSlideTexture(this, slide_textures[level]);
				}
				else
				{
					HideSlides(this);

					UE_LOG(LogTemp, Display, TEXT("Loading level number %d: %s, current level: %s"), level, *sublevels[level].ToString(),
					       *current_level.ToString());
					UGameplayStatics::LoadStreamLevel(this, sublevels[level], true, true, FLatentActionInfo());
					current_level = sublevels[level];
				}
			}
		}
	}
	else
	{
		level = level_loaded;
		UE_LOG(LogTemp, Error, TEXT("Error loading level number %d"), level);
	}
}

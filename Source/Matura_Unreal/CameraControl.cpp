// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraControl.h"

#include "FlyCharacter.h"
#include "TrackingCamera.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Styling/SlateBrush.h"
#include "Components/CanvasPanelSlot.h"

ACameraControl::ACameraControl()
{
	PrimaryActorTick.bCanEverTick = true;
	SetShowMouseCursor(true);
}

void ACameraControl::Tick(float DeltaTime)
{
	FHitResult res;
	GetHitResultUnderCursor(ECollisionChannel::ECC_PhysicsBody,true, res);

	if (AFlyCharacter* fly = Cast<AFlyCharacter>(GetPawn()))
	{
		if (fly->hud_instance)
		{
			if (UImage* camera_preview = Cast<UImage>(fly->hud_instance->GetWidgetFromName("camera_preview")))
			{
				
				if (ATrackingCamera* camera = Cast<ATrackingCamera>(res.GetActor()))
				{
					FVector2D translation = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
					UE_LOG(LogTemp, Warning, TEXT("position: %f %f"), translation.X, translation.Y);

					if (auto slot = Cast<UCanvasPanelSlot>(camera_preview->Slot))
					{
						slot->SetSize(FVector2D(camera->camera_manager->cv_size.width, camera->camera_manager->cv_size.height));
						slot->SetPosition(translation);
					}
					
					camera_preview->Brush.SetResourceObject(camera->camera_texture_2d);
					camera_preview->SetVisibility(ESlateVisibility::Visible);

					if (WasInputKeyJustPressed(FKey("LeftMouseButton")))
						selected = camera;
					
				} else
				{
					if (WasInputKeyJustPressed(FKey("LeftMouseButton")))
						selected = nullptr;
					
					if (selected)
					{
						camera_preview->Brush.SetResourceObject(selected->camera_texture_2d);
						camera_preview->SetVisibility(ESlateVisibility::Visible);
					} else
					{
						camera_preview->SetVisibility(ESlateVisibility::Hidden);
					}
				}
				
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Couldn't find widget with name: camera_preview"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Hud instance is null : CameraControl"));
	}
}

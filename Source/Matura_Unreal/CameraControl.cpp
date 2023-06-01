// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraControl.h"

#include <Engine/UserInterfaceSettings.h>

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
	GetHitResultUnderCursor(ECollisionChannel::ECC_PhysicsBody, true, res);


	if (AFlyCharacter* fly = Cast<AFlyCharacter>(GetPawn()))
	{
		if (fly->hud_instance)
		{
			if (UImage* camera_preview = Cast<UImage>(fly->hud_instance->GetWidgetFromName("camera_preview")))
			{
				if (ATrackingCamera* camera = Cast<ATrackingCamera>(res.GetActor()))
				{
					FVector2D mouse_position = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
					FVector2D viewport_size;
					GetWorld()->GetGameViewport()->GetViewportSize(viewport_size);
					float dpi = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(
						FIntPoint(viewport_size.X, viewport_size.Y));
					viewport_size /= dpi;

					UE_LOG(LogTemp, Display, TEXT("%f %f / %f %f"), mouse_position.X, mouse_position.Y, viewport_size.X, viewport_size.Y);

					camera_preview->SetRenderTransformAngle(round(camera->GetActorRotation().Roll / 90) * 90);
					bool swapped = int(camera_preview->GetRenderTransformAngle() / 90) % 2 != 0;

					FVector2D size = {double(camera->cv_size.width) / double(camera->cv_size.height), 1};

					if (swapped)
						swap(size.X, size.Y);

					double area = size.X * size.Y;
					double target_area = viewport_size.X * viewport_size.Y / 5;

					size *= sqrt(target_area / area);

					if (size.X > viewport_size.X)
						size /= size.X / (viewport_size.X);

					if (size.Y > viewport_size.Y)
						size /= size.Y / (viewport_size.Y);

					auto [trash, corner] = max({
						std::pair{(mouse_position - FVector2D{0, 0}).Size(), 0},
						std::pair{(mouse_position - FVector2D{viewport_size.X, 0}).Size(), 1},
						std::pair{(mouse_position - FVector2D{0, viewport_size.Y}).Size(), 2},
						std::pair{(mouse_position - FVector2D{viewport_size.X, viewport_size.Y}).Size(), 3},
					});
					
					if (auto slot = Cast<UCanvasPanelSlot>(camera_preview->Slot))
					{
						FVector2D position;
						position.X = (corner & 1 ? viewport_size.X - size.X / 2 : size.X / 2);
						position.Y = (corner & 2 ? viewport_size.Y - size.Y / 2 : size.Y / 2);
						slot->SetPosition(position);

						if (swapped)
							swap(size.X, size.Y);

						slot->SetSize(size);
					}

					if (WasInputKeyJustPressed(FKey("LeftMouseButton")))
						selected = camera;
				}
				else
				{
					if (WasInputKeyJustPressed(FKey("LeftMouseButton")))
						selected = nullptr;
				}

				if (selected)
				{
					auto brush = camera_preview->GetBrush();
					brush.SetResourceObject(selected->camera_texture_2d);
					camera_preview->SetBrush(brush);
					camera_preview->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					camera_preview->SetVisibility(ESlateVisibility::Hidden);
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

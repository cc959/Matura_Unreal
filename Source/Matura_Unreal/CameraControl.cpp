// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraControl.h"

#include <Engine/UserInterfaceSettings.h>

#include "FlyCharacter.h"
#include "TrackingCamera.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Styling/SlateBrush.h"
#include "Components/BackgroundBlur.h"
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

	FVector2D mouse_position = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	FVector2D viewport_size;
	GetWorld()->GetGameViewport()->GetViewportSize(viewport_size);
	float dpi = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(
		FIntPoint(viewport_size.X, viewport_size.Y));
	viewport_size /= dpi;

	if (AFlyCharacter* fly = Cast<AFlyCharacter>(GetPawn()))
	{
		if (fly->hud_instance)
		{
			if (UImage* camera_preview = Cast<UImage>(fly->hud_instance->GetWidgetFromName("camera_preview")))
			{
				if (UBackgroundBlur* border = Cast<UBackgroundBlur>(fly->hud_instance->GetWidgetFromName("border")))
				{

					ATrackingCamera* camera = Cast<ATrackingCamera>(res.GetActor());

					if (WasInputKeyJustPressed(FKey("LeftMouseButton")))
					{
						if (camera)
						{
							selected = camera;

							auto [trash, c] = max({
								std::pair{(mouse_position - FVector2D{0, 0}).Size(), 0},
								std::pair{(mouse_position - FVector2D{viewport_size.X, 0}).Size(), 1},
								std::pair{(mouse_position - FVector2D{0, viewport_size.Y}).Size(), 2},
								std::pair{(mouse_position - FVector2D{viewport_size.X, viewport_size.Y}).Size(), 3},
							});

							corner = c;
						}
					}
					
					if (selected)
					{
						camera_preview->SetRenderTransformAngle(round(selected->GetActorRotation().Roll / 90) * 90);
						bool swapped = int(camera_preview->GetRenderTransformAngle() / 90) % 2 != 0;

						FVector2D size = {double(selected->cv_size.width) / double(selected->cv_size.height), 1};

						if (swapped)
							swap(size.X, size.Y);

						double area = size.X * size.Y;
						double target_area = viewport_size.X * viewport_size.Y / 5;

						size *= sqrt(target_area / area);

						if (size.X > viewport_size.X - 50)
							size /= size.X / (viewport_size.X - 50);

						if (size.Y > viewport_size.Y - 50)
							size /= size.Y / (viewport_size.Y - 50);

						auto brush = camera_preview->GetBrush();
						brush.SetResourceObject(selected->camera_texture_2d);
						camera_preview->SetBrush(brush);
						camera_preview->SetVisibility(ESlateVisibility::Visible);

						if (auto slot = Cast<UCanvasPanelSlot>(camera_preview->Slot))
						{
							FVector2D position;
							position.X = (corner & 1 ? viewport_size.X - size.X / 2 - 25 : size.X / 2 + 25);
							position.Y = (corner & 2 ? viewport_size.Y - size.Y / 2 - 25 : size.Y / 2 + 25);
							slot->SetPosition(position);

							if (swapped)
								swap(size.X, size.Y);

							slot->SetSize(size);
						}

						border->SetVisibility(ESlateVisibility::Visible);
						
						if (auto slot = Cast<UCanvasPanelSlot>(border->Slot))
						{
							if (swapped)
								swap(size.X, size.Y);
							
							FVector2D position;
							position.X = (corner & 1 ? viewport_size.X - size.X / 2 - 25 : size.X / 2 + 25);
							position.Y = (corner & 2 ? viewport_size.Y - size.Y / 2 - 25 : size.Y / 2 + 25);
							slot->SetPosition(position);
							
							slot->SetSize(size + FVector2D(40, 40));
						}
						
						
						FVector2D position;
						position.X = (corner & 1 ? viewport_size.X - size.X / 2 - 25 : size.X / 2 + 25);
						position.Y = (corner & 2 ? viewport_size.Y - size.Y / 2 - 25 : size.Y / 2 + 25);
						
						if (abs(mouse_position.X - position.X) <= size.X && abs(mouse_position.Y - position.Y) <= size.Y)
						{
							if (WasInputKeyJustPressed(FKey("LeftMouseButton")))
							{
								if (selected->debug_frame_type == None)
									selected->debug_frame_type = Threshold;
								else if (selected->debug_frame_type == Threshold)
									selected->debug_frame_type = HueOnly;
								else if (selected->debug_frame_type == HueOnly)
									selected->debug_frame_type = SatOnly;
								else if (selected->debug_frame_type == SatOnly)
									selected->debug_frame_type = ValOnly;
								else if (selected->debug_frame_type == ValOnly)
									selected->debug_frame_type = None;

								selected->draw_debug_overlay = selected->debug_frame_type == None;
							}

							if (WasInputKeyJustPressed(FKey("RightMouseButton")))
							{
								selected->apply_threshold_to_debug_frame = !selected->apply_threshold_to_debug_frame;
							}

							if (WasInputKeyJustPressed(FKey("MiddleMouseButton")))
							{
								if (selected->learning_rate == -1)
									selected->learning_rate = 1;
								else if (selected->learning_rate != -1)
									selected->learning_rate = -1;
							}
						} else if (WasInputKeyJustPressed(FKey("LeftMouseButton")) && !camera)
						{
							selected->debug_frame_type = None;
							selected->draw_debug_overlay = true;
							selected->learning_rate = -1;
							selected->apply_threshold_to_debug_frame = false;
							
							selected = nullptr;
						}
					}
					else
					{
						camera_preview->SetVisibility(ESlateVisibility::Hidden);
						border->SetVisibility(ESlateVisibility::Hidden);
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Couldn't find widget with name: border"));
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

// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraControl.h"
#include "Tag.h"

ACameraControl::ACameraControl()
{
	SetShowMouseCursor(true);
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ACameraControl::Tick(float DeltaTime)
{
	if (WasInputKeyJustPressed(FKey("LeftMouseButton")))
	{
		FHitResult res;
		GetHitResultUnderCursorByChannel(ETraceTypeQuery::TraceTypeQuery1, true, res);

		if (res.GetActor())
		{
			UE_LOG(LogTemp, Warning, TEXT("Something was clicked!"));
		}

		ATag *tag = Cast<ATag>(res.GetActor());
		if (tag)
		{
			UE_LOG(LogTemp, Warning, TEXT("%d"), tag->tag_family);
		}
	}
}
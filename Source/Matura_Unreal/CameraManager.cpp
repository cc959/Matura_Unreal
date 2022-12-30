// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraManager.h"

CameraManager::CameraManager()
{
	Thread = FRunnableThread::Create(this, TEXT("Camera Thread"));

	
}

CameraManager::~CameraManager()
{
	if (Thread)
	{
		bRunThread = false;
		Thread->Kill();
		delete Thread;
	}
}

bool CameraManager::Init()
{

	return FRunnable::Init();
}

uint32 CameraManager::Run()
{
	while (bRunThread)
	{
		// TODO: Manage tracking cameras in scene
	}
	bThreadStopped = true;

	return 0;
}

void CameraManager::Stop()
{
	bRunThread = false;

	while (!bThreadStopped) { usleep(1); }
	
	FRunnable::Stop();
}


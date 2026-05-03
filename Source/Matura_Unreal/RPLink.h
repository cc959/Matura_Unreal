// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GlobalIncludes.h"

#include <vector>
#include <functional>
#include <map>
#include <atomic>
#include <mutex>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Common/TcpListener.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "TCPMessages.h"
#include "TrackingCamera.h"
#include "CameraManager.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "packer.h"
#include "unpacker.h"
#pragma clang diagnostic pop

#include "RPLink.generated.h"

UCLASS()
class MATURA_UNREAL_API ARPLink : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARPLink();

	bool ConnectToServer();

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FSocket *socket = nullptr;
	bool loop_running = true;
	std::mutex send_mutex_;
	std::vector<class ATrackingCamera *> active_cameras;
	class CameraManager *camera_manager = nullptr;

	// Connection handling
	void ConnectionLoop();

	// Message handling
	void ProcessReceivedData(const std::vector<uint8_t> &data);
	void ProcessMessage(const TCPMessages::Message &msg);

	// Message sending
	template <typename T>
	bool SendMessage(TCPMessages::MessageType type, const T &content);
	bool SendRawMessage(const TCPMessages::Message &msg);

	// Helper function to extract camera parameters
	TCPMessages::_CameraParams ExtractCameraParams(const class ATrackingCamera &camera);

	// Callback handlers

	void BallDetectionCallback(const TCPMessages::BallDetectionResponse &response);
	void TagDetectionCallback(const TCPMessages::TagDetectionResponse &response);
	void DebugFrameCallback(const TCPMessages::DebugFrameResponse &response);

public:
	FTransform camera_transform;

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	void SetCameras(const std::vector<class ATrackingCamera *> &cameras, class CameraManager *camera_manager);
	bool StartCameras();
	void StopCameras();
	void CameraParamUpdate();
};

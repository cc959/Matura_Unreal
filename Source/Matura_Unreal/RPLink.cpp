// Fill out your copyright notice in the Description page of Project Settings.

#include "RPLink.h"
#include <string>
#include <vector>
#include <sstream>

// Sets default values
ARPLink::ARPLink()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Attempt to connect to the server
bool ARPLink::ConnectToServer()
{
	// Create socket as a client
	if (socket)
	{
		socket->Close();
		socket = nullptr;
	}

	socket = FTcpSocketBuilder(TEXT("RPLinkClient"))
				 .AsReusable()
				 .AsBlocking()
				 .WithSendBufferSize(8192 * 1024)
				 .WithReceiveBufferSize(8192 * 1024);

	// Connect to the server
	auto endpoint = FIPv4Endpoint(FIPv4Address(127, 0, 0, 1), 8080);
	ISocketSubsystem *SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedRef<FInternetAddr> addr = SocketSubsystem->CreateInternetAddr();
	addr->SetIp(endpoint.Address.Value);
	addr->SetPort(endpoint.Port);

	bool connected = socket->Connect(*addr);
	if (connected)
	{
		LogDisplay(TEXT("Connected to server at %s:%d"), *endpoint.Address.ToString(), endpoint.Port);
	}
	else
	{
		LogDisplay(TEXT("Failed to connect to server at %s:%d"), *endpoint.Address.ToString(), endpoint.Port);
		socket = nullptr;
	}

	return connected;
}

// Called when the game starts or when spawned
void ARPLink::BeginPlay()
{
	Super::BeginPlay();

	Async(EAsyncExecution::Thread, [&]
		  { ConnectionLoop(); });
}

void ARPLink::CameraParamUpdate()
{
	int camera_id = 0;
	for (auto camera : active_cameras)
	{
		TCPMessages::_CameraParams params = ExtractCameraParams(*camera);

		TCPMessages::UpdateCameraParamsRequest request;
		request.camera_id = camera_id;
		request.params = params;

		SendMessage(TCPMessages::MessageType::UPDATE_CAMERA_PARAMS_REQ, request);

		camera_id++;
	}
}

void ARPLink::ConnectionLoop()
{
	bool connected = false;
	int64_t last_camera_params_update = NOW.count();

	while (loop_running)
	{
		if (socket && socket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
		{
			if (!connected)
			{
				usleep(1000000);
				connected = true;
				StartCameras();
			}

			if (NOW.count() - last_camera_params_update > 1e9 / 5)
			{
				CameraParamUpdate();
				last_camera_params_update = NOW.count();
			}

			uint32 PendingDataSize = 0;
			socket->HasPendingData(PendingDataSize);
			if (PendingDataSize > 0)
			{
				// Read message size first (32-bit integer)
				uint32 message_size = 0;
				int32 BytesRead = 0;
				if (socket->Recv(reinterpret_cast<uint8 *>(&message_size), sizeof(message_size), BytesRead, ESocketReceiveFlags::WaitAll))
				{
					if (BytesRead == sizeof(message_size))
					{
						if (message_size > 8000000 || message_size < 0)
						{
							LogDisplay(TEXT("Message size is too large: %d"), message_size);
						}
						else
						{
							// Now read the message data
							std::vector<uint8> buffer(message_size);
							auto total_bytes_read = 0;

							while (total_bytes_read < message_size)
							{
								if (!socket->Recv(&buffer[total_bytes_read], message_size - total_bytes_read, BytesRead, ESocketReceiveFlags::WaitAll))
								{
									LogDisplay(TEXT("Failed to read message data"));
									break;
								}
								total_bytes_read += BytesRead;

								if (total_bytes_read != message_size)
									LogDisplay(TEXT("Read incomplete message. %d of %d"), total_bytes_read, message_size);
							}

							if (BytesRead == message_size)
							{
								// Process the message data
								ProcessReceivedData(buffer);
							}
							else
							{
								LogDisplay(TEXT("Failed to read complete message. Expected %d bytes, got %d"), message_size, BytesRead);
							}
						}
					}
					else
					{
						LogDisplay(TEXT("Failed to read message size header"));
					}
				}
			}
		}
		else
		{
			if (connected)
			{
				connected = false;
				StopCameras();
			}

			// If disconnected, attempt to reconnect
			if (ConnectToServer())
			{
				LogDisplay(TEXT("Reconnected to server"));
			}
			else
			{
				LogDisplay(TEXT("Waiting to connect to server..."));
				usleep(1000000); // Wait a bit longer before next reconnection attempt
			}
			usleep(30000); // Short sleep to prevent busy waiting
		}
	}

	if (socket)
	{
		if (socket->Close())
		{
			LogDisplay(TEXT("Socket closed"));
		}
		else
		{
			LogDisplay(TEXT("Socket not closed"));
		}
		socket = nullptr;
	}
}

void ARPLink::ProcessReceivedData(const std::vector<uint8_t> &data)
{
	try
	{
		// Parse message using msgpack
		msgpack::unpacker unpk(data);

		TCPMessages::Message msg;
		unpk >> msg;

		// Process the message
		ProcessMessage(msg);
	}
	catch (const std::exception &e)
	{
		LogErr(TEXT("Error processing received data: %s"), UTF8_TO_TCHAR(e.what()));
	}
}

void ARPLink::ProcessMessage(const TCPMessages::Message &msg)
{
	try
	{
		switch (msg.type)
		{
		case TCPMessages::MessageType::BALL_DETECTION_RESP:
		{
			TCPMessages::BallDetectionResponse response;
			msg.parse(response);
			BallDetectionCallback(response);
			break;
		}

		case TCPMessages::MessageType::TAG_DETECTION_RESP:
		{
			TCPMessages::TagDetectionResponse response;
			msg.parse(response);
			TagDetectionCallback(response);
			break;
		}

		case TCPMessages::MessageType::DEBUG_FRAME_RESP:
		{
			TCPMessages::DebugFrameResponse response;
			msg.parse(response);
			DebugFrameCallback(response);
			break;
		}

		case TCPMessages::MessageType::STATUS_RESP:
		{
			TCPMessages::StatusResponse response;
			msg.parse(response);
			LogDisplay(TEXT("Status: %s - %s"),
					   (response.success ? TEXT("Success") : TEXT("Error")),
					   *FString(response.message.c_str()));
			break;
		}

		default:
			LogWarning(TEXT("Received unknown message type: %d"), static_cast<int>(msg.type));
			break;
		}
	}
	catch (const std::exception &e)
	{
		LogErr(TEXT("Error processing message: %s"), UTF8_TO_TCHAR(e.what()));
	}
}

void ARPLink::BallDetectionCallback(const TCPMessages::BallDetectionResponse &response)
{
	if (camera_manager)
	{
		if (response.camera_id < active_cameras.size())
		{
			ATrackingCamera *camera = active_cameras[response.camera_id];
			if (camera)
			{
				CameraManager::Detection detection;
				detection.position = Point2d(response.position.first, response.position.second);
				detection.time = response.timestamp;
				detection.camera_id = response.camera_id;
				camera->ball = detection.position;
				camera_manager->event_passer.push(detection);
				// LogDisplay(TEXT("Ball detection callback %d %f %f %f"), response.camera_id, response.position.first, response.position.second, response.timestamp);
			}
			else
			{
				LogDisplay(TEXT("Camera %d not found"), response.camera_id);
			}
		}
		else
		{
			LogDisplay(TEXT("Camera id out of range %d"), response.camera_id);
		}
	}
	else
	{
		LogDisplay(TEXT("Camera manager not found"));
	}
}

void ARPLink::TagDetectionCallback(const TCPMessages::TagDetectionResponse &response)
{
	if (response.camera_id < active_cameras.size())
	{
		ATrackingCamera *camera = active_cameras[response.camera_id];
		if (camera)
		{
			auto [camera_transform, local_tag_transforms] = camera->ApplyTagPoses(response.detections);
			if (!camera_transform.Equals(FTransform::Identity))
			{
				camera->UpdateTransform(camera_transform);

				for (auto [tag, local_transform] : local_tag_transforms)
				{
					FMatrix world_transform = (local_transform * FQuat::MakeFromEuler(FVector(0, 0, 90)).ToMatrix() *
											   FQuat::MakeFromEuler(FVector(0, 90, 0)).ToMatrix()) *
											  camera->camera_transform.ToMatrixNoScale();
					tag->UpdateTransform(FTransform(world_transform));
				}
			}
			else
			{
				LogDisplay(TEXT("Camera %d has no transform"), response.camera_id);
			}
		}
		else
		{
			LogDisplay(TEXT("Camera %d not found"), response.camera_id);
		}
	}
	// LogDisplay(TEXT("Tag detection callback %d %d"), response.camera_id, response.detections.size());
}

inline cv::Mat bytesToMat(const std::string &bytes, int width, int height, int type)
{
	cv::Mat mat(height, width, type);
	if (!bytes.empty())
	{
		if (mat.isContinuous())
		{
			std::memcpy(mat.data, &bytes[0], bytes.size());
		}
		else
		{
			size_t rowBytes = width * mat.elemSize();
			for (int i = 0; i < height; ++i)
			{
				std::memcpy(mat.ptr(i), bytes.data() + i * rowBytes, rowBytes);
			}
		}
	}
	return mat;
}

void ARPLink::DebugFrameCallback(const TCPMessages::DebugFrameResponse &response)
{
	if (response.camera_id < active_cameras.size())
	{
		ATrackingCamera *camera = active_cameras[response.camera_id];
		if (camera)
		{
			cv::Mat mat = bytesToMat(response.frame_data, response.width, response.height, response.type);
			lock_guard<mutex> lock(camera->debug_frame_mutex);
			camera->cv_debug_frame = mat;
			// LogDisplay(TEXT("Debug frame callback %d %d"), response.camera_id, response.frame_data.size());
		}
	}
}

template <typename T>
bool ARPLink::SendMessage(TCPMessages::MessageType type, const T &content)
{
	LogDisplay(TEXT("Sending message of type %d"), static_cast<int>(type));
	TCPMessages::Message msg = TCPMessages::Message::create(type, content);
	return SendRawMessage(msg);
}

bool ARPLink::SendRawMessage(const TCPMessages::Message &msg)
{
	try
	{
		std::lock_guard<std::mutex> lock(send_mutex_);

		if (!socket || socket->GetConnectionState() != ESocketConnectionState::SCS_Connected)
		{
			return false;
		}

		// Serialize the message using msgpack
		msgpack::packer pk;
		pk << msg;
		std::vector<uint8_t> data = pk.get_buffer();

		// Prepare header (size)
		uint32 message_size = data.size();

		LogDisplay(TEXT("Sending message of size %d"), message_size);

		// Send header
		int32 BytesSent = 0;
		socket->Send(reinterpret_cast<uint8 *>(&message_size), sizeof(message_size), BytesSent);

		if (BytesSent != sizeof(message_size))
		{
			LogWarning(TEXT("Failed to send message size header"));
			return false;
		}

		// Send message
		socket->Send(data.data(), data.size(), BytesSent);

		if (BytesSent != data.size())
		{
			LogWarning(TEXT("Failed to send complete message. Sent %d of %d bytes"), BytesSent, data.size());
			return false;
		}

		return true;
	}
	catch (const std::exception &e)
	{
		LogErr(TEXT("Error sending message: %s"), UTF8_TO_TCHAR(e.what()));
		return false;
	}
}

// Helper function to extract camera parameters from ATrackingCamera
TCPMessages::_CameraParams ARPLink::ExtractCameraParams(const ATrackingCamera &camera)
{
	TCPMessages::_CameraParams params;
	params.tag_families.clear();

	vector<bool> already_added(10, false);
	for (auto tag : camera.april_tags)
	{
		auto tag_family = static_cast<TCPMessages::_TagFamily>(tag->tag_family.GetValue());

		if (!already_added[tag_family])
		{
			params.tag_families.push_back(tag_family);
			already_added[tag_family] = true;
		}
	}

	// Set camera parameters from the TrackingCamera object
	params.camera_path = std::string(TCHAR_TO_UTF8(*camera.camera_path));
	params.resolution = std::make_pair(camera.resolution.X, camera.resolution.Y);
	params.processing_resolution_factor = camera.processing_resolution_factor;
	params.exposure = camera.exposure;
	params.focal_length = std::make_pair(camera.focal_length.X, camera.focal_length.Y);
	params.k_twins = std::make_pair(camera.k_twins.X, camera.k_twins.Y);
	params.p_twins = std::make_pair(camera.p_twins.X, camera.p_twins.Y);
	params.detection_type = static_cast<TCPMessages::_DetectionType>(camera.detection_type.GetValue());
	params.debug_frame_type = static_cast<TCPMessages::_DebugFrameType>(camera.debug_frame_type.GetValue());
	params.draw_debug_overlay = camera.draw_debug_overlay;
	params.debug_output = camera.debug_output;

	// HSV thresholds
	params.low_H = camera.low_H;
	params.high_H = camera.high_H;
	params.low_S = camera.low_S;
	params.high_S = camera.high_S;
	params.low_V = camera.low_V;
	params.high_V = camera.high_V;

	// Background subtraction and blob detection settings
	params.learning_rate = camera.learning_rate;
	params.min_blob_size = camera.min_blob_size;

	return params;
}

void ARPLink::SetCameras(const std::vector<ATrackingCamera *> &cameras, class CameraManager *manager)
{
	active_cameras = cameras;
	camera_manager = manager;
}

bool ARPLink::StartCameras()
{
	// Convert Unreal camera array to std::vector of CameraParams
	std::vector<TCPMessages::_CameraParams> params;

	for (ATrackingCamera *camera : active_cameras)
	{
		if (!camera)
			continue;

		camera->must_update_tags = true;
		while (camera->must_update_tags)
			usleep(10000); // wait 10 ms

		// Extract parameters from the camera
		TCPMessages::_CameraParams camera_param = ExtractCameraParams(*camera);

		camera->in_use = true;

		// Add to parameters
		params.push_back(camera_param);
	}

	// Create and send request
	TCPMessages::StartCamerasRequest request;
	request.camera_params = params;

	return SendMessage(TCPMessages::MessageType::START_CAMERAS_REQ, request);
}

void ARPLink::StopCameras()
{
	for (ATrackingCamera *camera : active_cameras)
	{
		if (camera)
		{
			camera->in_use = false;
			camera->used_ball = Point2d{-1, -1};
			camera->ball = Point2d{-1, -1};
		}
	}
}

// Called every frame
void ARPLink::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARPLink::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	loop_running = false;
	Super::EndPlay(EndPlayReason);
}

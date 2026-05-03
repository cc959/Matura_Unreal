#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>
#include <vector>
#include <opencv2/opencv.hpp>
#include "packer.h"
#include "unpacker.h"
#include "TCPMessages.h"
#include "GlobalIncludes.h"

using boost::asio::ip::tcp;
using namespace TCPMessages;
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

class TcpClient
{
private:
	boost::asio::io_context &io_context_;
	tcp::socket socket_;
	std::thread receive_thread_;
	std::atomic<bool> running_{false};
	std::mutex send_mutex_;

	// Callback handlers
	std::function<void(const BallDetectionResponse &)> ball_detection_callback_;
	std::function<void(const TagDetectionResponse &)> tag_detection_callback_;
	std::function<void(const DebugFrameResponse &)> debug_frame_callback_;

public:
	TcpClient(boost::asio::io_context &io_context, const std::string &server_ip, int server_port)
		: io_context_(io_context), socket_(io_context)
	{
		try
		{
			tcp::endpoint endpoint(boost::asio::ip::make_address(server_ip), server_port);
			socket_.connect(endpoint);
			running_ = true;
			LogDisplay("Connected to server: %s:%d", server_ip.c_str(), server_port);

			// Start receive thread
			receive_thread_ = std::thread([this]()
										  { this->receive_messages(); });
		}
		catch (std::exception &e)
		{
			LogErr("Connection error: %s", e.what());
		}
	}

	~TcpClient()
	{
		disconnect();
	}

	void disconnect()
	{
		running_ = false;

		if (socket_.is_open())
		{
			boost::system::error_code ec;
			socket_.close(ec);
		}

		if (receive_thread_.joinable())
		{
			receive_thread_.join();
		}
	}

	// Set callback handlers
	void set_ball_detection_callback(std::function<void(const BallDetectionResponse &)> callback)
	{
		ball_detection_callback_ = callback;
	}

	void set_tag_detection_callback(std::function<void(const TagDetectionResponse &)> callback)
	{
		tag_detection_callback_ = callback;
	}

	void set_debug_frame_callback(std::function<void(const DebugFrameResponse &)> callback)
	{
		debug_frame_callback_ = callback;
	}

	// Start cameras with specified parameters
	bool start_cameras(const std::vector<_CameraParams> &camera_params)
	{
		StartCamerasRequest request;
		request.camera_params = camera_params;

		return send_message(MessageType::START_CAMERAS_REQ, request);
	}

	// Set debug frame type for a camera
	bool set_debug_frame_type(int camera_id, _DebugFrameType type)
	{
		SetDebugFrameTypeRequest request;
		request.camera_id = camera_id;
		request.debug_frame_type = type;

		return send_message(MessageType::SET_DEBUG_FRAME_TYPE_REQ, request);
	}

	// Update camera parameters
	bool update_camera_params(int camera_id, const _CameraParams &params)
	{
		UpdateCameraParamsRequest request;
		request.camera_id = camera_id;
		request.params = params;

		LogDisplay("Sending update for camera %d with HSV thresholds: [%d, %d, %d] - [%d, %d, %d]",
				   camera_id, params.low_H, params.low_S, params.low_V, params.high_H, params.high_S, params.high_V);

		return send_message(MessageType::UPDATE_CAMERA_PARAMS_REQ, request);
	}

private:
	// Receive and process messages from server
	void receive_messages()
	{
		try
		{
			while (running_ && socket_.is_open())
			{
				boost::system::error_code error;

				// Read message size
				uint32_t message_size = 0;
				boost::asio::read(socket_, boost::asio::buffer(&message_size, sizeof(message_size)), error);

				if (error)
				{
					if (error == boost::asio::error::eof)
					{
						LogDisplay("Server disconnected");
					}
					else
					{
						LogWarning("Error reading message size: %s", error.message().c_str());
					}
					break;
				}

				// Read message data
				std::vector<uint8_t> data(message_size);
				boost::asio::read(socket_, boost::asio::buffer(data), error);

				if (error)
				{
					LogWarning("Error reading message data: %s", error.message().c_str());
					break;
				}

				// Parse the message using msgpack
				msgpack::unpacker unpk(data);

				Message msg;
				unpk >> msg;

				// Process the message based on type
				process_message(msg);
			}
		}
		catch (std::exception &e)
		{
			LogErr("Error in receive_messages: %s", e.what());
		}

		running_ = false;
	}

	// Process received messages
	void process_message(const Message &msg)
	{
		try
		{
			switch (msg.type)
			{
			case MessageType::BALL_DETECTION_RESP:
			{
				if (ball_detection_callback_)
				{
					BallDetectionResponse response;
					msg.parse(response);
					ball_detection_callback_(response);
				}
				break;
			}

			case MessageType::TAG_DETECTION_RESP:
			{
				if (tag_detection_callback_)
				{
					TagDetectionResponse response;
					msg.parse(response);
					tag_detection_callback_(response);
				}
				break;
			}

			case MessageType::DEBUG_FRAME_RESP:
			{
				if (debug_frame_callback_)
				{
					DebugFrameResponse response;
					msg.parse(response);
					debug_frame_callback_(response);
				}
				break;
			}

			case MessageType::STATUS_RESP:
			{
				StatusResponse response;
				msg.parse(response);
				LogDisplay("Status: %s - %s",
						   (response.success ? "Success" : "Error"),
						   response.message.c_str());
				break;
			}

			default:
				LogWarning("Received unknown message type: %d", static_cast<int>(msg.type));
				break;
			}
		}
		catch (std::exception &e)
		{
			LogErr("Error processing message: %s", e.what());
		}
	}

	// Send a message of any type
	template <typename T>
	bool send_message(MessageType type, const T &content)
	{
		LogDisplay("Sending message of type %d", static_cast<int>(type));
		Message msg = Message::create(type, content);
		return send_raw_message(msg);
	}

	// Send a raw message
	bool send_raw_message(const Message &msg)
	{
		try
		{
			std::lock_guard<std::mutex> lock(send_mutex_);

			if (!socket_.is_open())
			{
				return false;
			}

			// Serialize the message using msgpack
			msgpack::packer pk;
			pk << msg;
			std::vector<uint8_t> data = pk.get_buffer();

			// Prepare header (size)
			uint32_t message_size = data.size();

			LogDisplay("Sending message of size %d", message_size);

			// Send header
			boost::asio::write(socket_, boost::asio::buffer(&message_size, sizeof(message_size)));

			// Send message
			boost::asio::write(socket_, boost::asio::buffer(data));

			return true;
		}
		catch (std::exception &e)
		{
			LogErr("Error sending message: %s", e.what());
			return false;
		}
	}
};

// Example usage of the client
int main()
{
	try
	{
		boost::asio::io_context io_context;

		// Create client and connect to server
		TcpClient client(io_context, "192.168.2.1", 8080);
		// TcpClient client(io_context, "127.0.0.1", 8080);
		// Set up callbacks
		client.set_ball_detection_callback([](const BallDetectionResponse &response)
										   {
											   // LogDisplay("Ball detected at (%f, %f) from camera %d",
											   // response.position.x,
											   // response.position.y,
											   // response.camera_id);
										   });

		client.set_tag_detection_callback([](const TagDetectionResponse &response)
										  {
											  // LogDisplay("Received %zu tag detections from camera %d",
											  //    response.detections.size(),
											  //    response.camera_id);
										  });

		client.set_debug_frame_callback([](const DebugFrameResponse &response)
										{
            // Convert bytes back to Mat
            cv::Mat frame = bytesToMat(response.frame_data, response.width, response.height, response.type);
            
            // Display the frame
            cv::imshow("Debug Frame " + std::to_string(response.camera_id), frame);
			cv::resizeWindow("Debug Frame " + std::to_string(response.camera_id), 800, 600);
			cv::waitKey(1); });

		// Start io_context in a separate thread
		std::thread io_thread([&io_context]()
							  { io_context.run(); });

		// Create camera parameters
		std::vector<_CameraParams> camera_params;

		// Camera 1
		_CameraParams camera1;
		camera1.camera_path = "/dev/video0";
		camera1.resolution = {640, 480};
		camera1.fps = 206.65;
		camera1.learning_rate = 1;
		camera1.processing_resolution_factor = 1;

		// Camera 2
		_CameraParams camera2;
		camera2.camera_path = "/dev/video8";
		camera2.resolution = {640, 480};
		camera2.fps = 206.65;
		camera2.learning_rate = 1;
		camera2.processing_resolution_factor = 1;

		camera_params.push_back(camera1);
		camera_params.push_back(camera2);

		// Keep track of the active camera configurations
		std::vector<_CameraParams> active_params = camera_params;

		// Start cameras
		client.start_cameras(camera_params);

		// Main loop
		std::string command;
		while (true)
		{
			printf("Enter command (quit, debug1, debug2, params1, params2): ");
			std::getline(std::cin, command);

			if (command == "quit")
			{
				break;
			}
			else if (command == "debug1")
			{
				// Change debug frame type for camera 0
				int debug_type;
				printf("Enter debug frame type (0-5): ");
				std::cin >> debug_type;
				std::cin.ignore();

				client.set_debug_frame_type(0, static_cast<_DebugFrameType>(debug_type));

				// Update stored parameters
				if (debug_type >= 0 && debug_type <= 5)
				{
					active_params[0].debug_frame_type = static_cast<_DebugFrameType>(debug_type);
				}
			}
			else if (command == "debug2")
			{
				// Change debug frame type for camera 1
				int debug_type;
				printf("Enter debug frame type (0-5): ");
				std::cin >> debug_type;
				std::cin.ignore();

				client.set_debug_frame_type(1, static_cast<_DebugFrameType>(debug_type));

				// Update stored parameters
				if (debug_type >= 0 && debug_type <= 5)
				{
					active_params[1].debug_frame_type = static_cast<_DebugFrameType>(debug_type);
				}
			}
			else if (command == "params1" || command == "params2")
			{
				// Update camera parameters
				int camera_id = (command == "params1") ? 0 : 1;

				// Get current camera parameters
				_CameraParams params = active_params[camera_id];

				// Helper function to get input with default value support
				auto getIntInput = [](const std::string &prompt, int defaultValue) -> int
				{
					std::string input;
					printf("%s (current: %d): ", prompt.c_str(), defaultValue);
					std::getline(std::cin, input);
					if (input.empty())
					{
						return defaultValue;
					}
					return std::stoi(input);
				};

				auto getFloatInput = [](const std::string &prompt, float defaultValue) -> float
				{
					std::string input;
					printf("%s (current: %.2f): ", prompt.c_str(), defaultValue);
					std::getline(std::cin, input);
					if (input.empty())
					{
						return defaultValue;
					}
					return std::stof(input);
				};

				auto getBoolInput = [](const std::string &prompt, bool defaultValue) -> bool
				{
					std::string input;
					printf("%s (current: %s) [y/n]: ", prompt.c_str(), defaultValue ? "yes" : "no");
					std::getline(std::cin, input);
					if (input.empty())
					{
						return defaultValue;
					}
					return (input == "y" || input == "Y" || input == "yes" || input == "YES" || input == "1");
				};

				// Get input for all parameters
				LogDisplay("Updating parameters for camera %d (press Enter to keep current value)", camera_id);

				// Processing parameters
				params.processing_resolution_factor = getFloatInput("Enter processing resolution factor", params.processing_resolution_factor);

				// HSV thresholds
				params.low_H = getIntInput("Enter low H (0-179)", params.low_H);
				params.high_H = getIntInput("Enter high H (0-179)", params.high_H);
				params.low_S = getIntInput("Enter low S (0-255)", params.low_S);
				params.high_S = getIntInput("Enter high S (0-255)", params.high_S);
				params.low_V = getIntInput("Enter low V (0-255)", params.low_V);
				params.high_V = getIntInput("Enter high V (0-255)", params.high_V);

				// Background subtraction and blob detection
				params.learning_rate = getFloatInput("Enter learning rate (-1 for default)", params.learning_rate);
				params.min_blob_size = getIntInput("Enter minimum blob size", params.min_blob_size);

				// Detection and debug settings
				printf("Detection type (0 for BlobDetector, 1 for ContourFilter)");
				params.detection_type = static_cast<_DetectionType>(getIntInput("", static_cast<int>(params.detection_type)));

				printf("Debug frame type (0=None, 1=Threshold, 2=HueOnly, 3=SatOnly, 4=ValOnly, 5=Preview)");
				params.debug_frame_type = static_cast<_DebugFrameType>(getIntInput("", static_cast<int>(params.debug_frame_type)));

				params.draw_debug_overlay = getBoolInput("Draw debug overlay", params.draw_debug_overlay);
				params.debug_output = getBoolInput("Enable debug output", params.debug_output);

				// Send the update request
				if (client.update_camera_params(camera_id, params))
				{
					active_params[camera_id] = params;
				}
			}
		}

		// Clean up
		client.disconnect();

		if (io_thread.joinable())
		{
			io_thread.join();
		}

		return 0;
	}
	catch (std::exception &e)
	{
		LogErr("Exception: %s", e.what());
		return 1;
	}
}
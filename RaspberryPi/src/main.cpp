#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <atomic>
#include <sstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "TrackingCamera.h"
#include "GlobalIncludes.h"
#include "CameraManager.h"
#include "TCPMessages.h"

using boost::asio::ip::tcp;

// Shared data between threads
struct SharedData
{
    FTransform lastTransform;
    std::mutex mutex;
    std::atomic<bool> hasNewTransform{false};
};

SharedData g_sharedData;

class TcpServer;
TcpServer *g_server = nullptr;

// Helper function to convert cv::Mat to/from byte vector for serialization
inline std::string matToBytes(const cv::Mat &mat)
{
    std::string bytes;
    if (!mat.empty())
    {
        bytes.resize(mat.total() * mat.elemSize());
        if (mat.isContinuous())
        {
            std::memcpy(&bytes[0], mat.data, bytes.size());
        }
        else
        {
            size_t rowBytes = mat.cols * mat.elemSize();
            for (int i = 0; i < mat.rows; ++i)
            {
                std::memcpy(bytes.data() + i * rowBytes, mat.ptr(i), rowBytes);
            }
        }
    }
    return bytes;
}

class TcpServer
{
private:
    boost::asio::io_context &io_context_;
    tcp::acceptor acceptor_;
    std::vector<ATrackingCamera *> cameras;
    CameraManager *camera_manager = nullptr;
    std::atomic<bool> is_running{false};
    std::thread ball_detection_thread;
    std::thread tag_detection_thread;
    std::thread debug_frame_thread;
    std::shared_ptr<tcp::socket> active_client;
    std::mutex client_mutex;
    std::atomic<bool> has_client{false};

    void start_accept()
    {
        auto socket = std::make_shared<tcp::socket>(io_context_);

        acceptor_.async_accept(*socket,
                               [this, socket](const boost::system::error_code &error)
                               {
                                   if (!error)
                                   {
                                       // Check if we already have an active client
                                       std::lock_guard<std::mutex> lock(client_mutex);

                                       if (has_client.load())
                                       {
                                           // Refuse connection if a client is already connected
                                           LogWarning("Refusing connection from %s:%d - already have an active client",
                                                      socket->remote_endpoint().address().to_string().c_str(),
                                                      socket->remote_endpoint().port());

                                           boost::system::error_code ec;
                                           socket->close(ec);
                                       }
                                       else
                                       {
                                           // Accept the connection
                                           LogDisplay("Client connected: %s:%d",
                                                      socket->remote_endpoint().address().to_string().c_str(),
                                                      socket->remote_endpoint().port());

                                           // Set as active client
                                           active_client = socket;
                                           has_client.store(true);

                                           // Handle client in a separate thread
                                           std::thread([this, socket]()
                                                       { handle_client(socket); })
                                               .detach();
                                       }
                                   }

                                   // Continue accepting connections
                                   start_accept();
                               });
    }

    void handle_client(std::shared_ptr<tcp::socket> socket)
    {
        try
        {
            while (socket->is_open())
            {
                boost::system::error_code error;

                // Read message size (4 bytes)
                uint32_t message_size = 0;
                boost::asio::read(*socket, boost::asio::buffer(&message_size, sizeof(message_size)), error);

                if (error)
                {
                    if (error == boost::asio::error::eof)
                    {
                        LogDisplay("Client disconnected");
                    }
                    else
                    {
                        LogWarning("Error reading message size from client: %s", error.message().c_str());
                    }
                    break;
                }

                LogDisplay("Received message of size %d", message_size);

                // Read message data
                std::vector<uint8_t> message_data(message_size);

                boost::asio::read(*socket, boost::asio::buffer(message_data), error);

                if (error)
                {
                    LogWarning("Error reading message data from client: %s", error.message().c_str());
                    break;
                }

                // Parse the message using msgpack
                msgpack::unpacker unpk(message_data);
                Message msg;
                unpk >> msg;

                LogDisplay("Received message of type: %d", static_cast<int>(msg.type));

                // Process the message
                g_server->process_message(socket, msg);
            }
        }
        catch (std::exception &e)
        {
            LogErr("Exception in client handler: %s", e.what());
        }

        // Clear active client
        {
            stop_cameras();

            std::lock_guard<std::mutex> lock(g_server->client_mutex);
            g_server->active_client.reset();
            g_server->has_client.store(false);
            LogDisplay("Active client removed");
        }

        if (socket->is_open())
        {
            boost::system::error_code ec;
            socket->close(ec);
        }
    }

    // Method to process incoming messages
    void process_message(std::shared_ptr<tcp::socket> socket, Message msg)
    {
        LogDisplay("Processing message of type: %d", static_cast<int>(msg.type));

        switch (msg.type)
        {
        case MessageType::START_CAMERAS_REQ:
            handle_start_cameras_request(socket, msg);
            break;

        case MessageType::SET_DEBUG_FRAME_TYPE_REQ:
            handle_set_debug_frame_type_request(socket, msg);
            break;

        case MessageType::UPDATE_CAMERA_PARAMS_REQ:
            handle_update_camera_params_request(socket, msg);
            break;

        default:
            LogWarning("Unknown message type: %d", static_cast<int>(msg.type));
            break;
        }
    }

    // Handler for START_CAMERAS_REQ
    void handle_start_cameras_request(std::shared_ptr<tcp::socket> socket, const Message &msg)
    {
        try
        {
            // If cameras are already running, stop them first
            if (is_running.load())
            {
                stop_cameras();
            }

            // Parse the request
            StartCamerasRequest request;
            msg.parse(request);

            // Create cameras based on parameters
            for (const auto &params : request.camera_params)
            {

                LogDisplay("Creating camera with path: %s", params.camera_path.c_str());
                for (const auto &tag_family : params.tag_families)
                {
                    LogDisplay("Tag family: %d", static_cast<int>(tag_family));
                }

                auto camera = new ATrackingCamera(params.camera_path, params.tag_families);

                // Set all camera parameters
                camera->resolution = params.resolution;
                camera->fps = params.fps;
                camera->processing_resolution_factor = params.processing_resolution_factor;
                camera->exposure = params.exposure;
                camera->focal_length = params.focal_length;
                camera->k_twins = params.k_twins;
                camera->p_twins = params.p_twins;
                camera->detection_type = params.detection_type;
                camera->debug_frame_type = params.debug_frame_type;
                camera->draw_debug_overlay = params.draw_debug_overlay;
                camera->debug_output = params.debug_output;

                // Set HSV thresholds
                camera->low_H = params.low_H;
                camera->high_H = params.high_H;
                camera->low_S = params.low_S;
                camera->high_S = params.high_S;
                camera->low_V = params.low_V;
                camera->high_V = params.high_V;

                // Set background subtraction learning rate
                camera->learning_rate = params.learning_rate;

                // Set minimum blob size
                camera->min_blob_size = params.min_blob_size;

                cameras.push_back(camera);
            }

            // Create and start camera manager
            camera_manager = new CameraManager(cameras);
            camera_manager->Start();

            is_running.store(true);

            // Start threads for handling data streams
            start_data_streaming_threads();

            // Send success response
            send_status_response(socket, true, "Cameras initialized successfully");
        }
        catch (std::exception &e)
        {
            LogErr("Error starting cameras: %s", e.what());
            send_status_response(socket, false, std::string("Error starting cameras: ") + e.what());
        }
    }

    // Handler for SET_DEBUG_FRAME_TYPE_REQ
    void handle_set_debug_frame_type_request(std::shared_ptr<tcp::socket> socket, const Message &msg)
    {
        try
        {
            if (!is_running.load() || !camera_manager)
            {
                send_status_response(socket, false, "Cameras not running");
                return;
            }

            SetDebugFrameTypeRequest request;
            msg.parse(request);

            LogDisplay("Setting debug frame type for camera %d to %d", request.camera_id, request.debug_frame_type);

            // Update the debug frame type
            if (request.camera_id >= 0 && request.camera_id < cameras.size())
            {
                cameras[request.camera_id]->debug_frame_type = request.debug_frame_type;
                send_status_response(socket, true, "Debug frame type updated");
            }
            else
            {
                send_status_response(socket, false, "Invalid camera ID");
            }
        }
        catch (std::exception &e)
        {
            LogErr("Error setting debug frame type: %s", e.what());
            send_status_response(socket, false, std::string("Error setting debug frame type: ") + e.what());
        }
    }

    // Handler for UPDATE_CAMERA_PARAMS_REQ
    void handle_update_camera_params_request(std::shared_ptr<tcp::socket> socket, const Message &msg)
    {
        try
        {
            if (!is_running.load() || !camera_manager)
            {
                send_status_response(socket, false, "Cameras not running");
                return;
            }

            UpdateCameraParamsRequest request;
            msg.parse(request);

            LogDisplay("Updating parameters for camera %d", request.camera_id);

            // Update camera parameters
            if (request.camera_id >= 0 && request.camera_id < cameras.size())
            {
                ATrackingCamera *camera = cameras[request.camera_id];
                // Update processing parameters
                camera->processing_resolution_factor = request.params.processing_resolution_factor;

                // Update HSV thresholds
                camera->low_H = request.params.low_H;
                camera->high_H = request.params.high_H;
                camera->low_S = request.params.low_S;
                camera->high_S = request.params.high_S;
                camera->low_V = request.params.low_V;
                camera->high_V = request.params.high_V;

                // Update background subtraction learning rate
                camera->learning_rate = request.params.learning_rate;

                // Update minimum blob size
                camera->min_blob_size = request.params.min_blob_size;

                // Update detection type
                camera->detection_type = request.params.detection_type;

                // Update debug frame type and overlay settings
                camera->debug_frame_type = request.params.debug_frame_type;
                camera->draw_debug_overlay = request.params.draw_debug_overlay;

                camera->debug_output = request.params.debug_output;

                // Send success response
                send_status_response(socket, true, "Camera parameters updated successfully");
            }
            else
            {
                send_status_response(socket, false, "Invalid camera ID");
            }
        }
        catch (std::exception &e)
        {
            LogErr("Error updating camera parameters: %s", e.what());
            send_status_response(socket, false, std::string("Error updating camera parameters: ") + e.what());
        }
    }

    // Helper to send status response
    void send_status_response(std::shared_ptr<tcp::socket> socket, bool success, const std::string &message)
    {
        if (success)
        {
            LogDisplay("Status response: %s", message.c_str());
        }
        else
        {
            LogErr("Status response: %s", message.c_str());
        }
    }

    // Start threads for processing data streams
    void start_data_streaming_threads()
    {
        // Ball detection thread
        ball_detection_thread = std::thread([this]()
                                            { this->handle_ball_detection_stream(); });

        // Tag detection thread
        tag_detection_thread = std::thread([this]()
                                           { this->handle_tag_detection_stream(); });

        // Debug frame thread
        debug_frame_thread = std::thread([this]()
                                         { this->handle_debug_frame_stream(); });
    }

    // Handle ball detection stream
    void handle_ball_detection_stream()
    {
        while (is_running.load() && camera_manager)
        {

            _Detection detection;
            if (camera_manager->event_passer.pop(&detection))
            {
                // Create response
                BallDetectionResponse response;
                response.camera_id = detection.camera_id;
                response.position = detection.position;
                response.timestamp = detection.time;

                // Create message
                Message msg = Message::create(MessageType::BALL_DETECTION_RESP, response);

                // Send to client
                send_to_client(msg);
            }
        }
    }

    // Handle tag detection stream
    void handle_tag_detection_stream()
    {
        while (is_running.load() && camera_manager)
        {
            std::pair<int, std::vector<_TagDetection>> tag_data;
            if (camera_manager->tag_event_passer.pop(&tag_data))
            {
                // Create response
                TagDetectionResponse response;
                response.camera_id = tag_data.first;
                response.detections = tag_data.second;

                // Create message
                Message msg = Message::create(MessageType::TAG_DETECTION_RESP, response);

                // Send to client
                send_to_client(msg);
            }
        }
    }

    // Handle debug frame stream
    void handle_debug_frame_stream()
    {
        while (is_running.load() && camera_manager)
        {
            std::pair<int, cv::Mat> frame_data;
            if (camera_manager->debug_frame_passer.pop(&frame_data))
            {
                if (!frame_data.second.empty())
                {
                    // Create response
                    DebugFrameResponse response;
                    response.camera_id = frame_data.first;
                    response.frame_data = matToBytes(frame_data.second);
                    response.width = frame_data.second.cols;
                    response.height = frame_data.second.rows;
                    response.type = frame_data.second.type();

                    // Create message
                    Message msg = Message::create(MessageType::DEBUG_FRAME_RESP, response);

                    // Send to client
                    send_to_client(msg);
                }
            }
        }
    }

    // Helper to send a message
    bool send_message(std::shared_ptr<tcp::socket> socket, const Message &msg)
    {
        try
        {
            // Serialize message using msgpack
            msgpack::packer pk;
            pk << msg;

            // Get serialized data
            std::vector<uint8_t> data = pk.get_buffer();

            // Prepare message size
            uint32_t message_size = data.size();

            // LogDisplay("Sending message of size %d", message_size);

            // Send size and then data
            boost::asio::write(*socket, boost::asio::buffer(&message_size, sizeof(message_size)));
            boost::asio::write(*socket, boost::asio::buffer(data));

            return true;
        }
        catch (std::exception &e)
        {
            LogErr("Error sending message: %s", e.what());
            return false;
        }
    }

    // Send a message to the active client
    void send_to_client(const Message &msg)
    {
        std::lock_guard<std::mutex> lock(client_mutex);

        if (has_client.load() && active_client && active_client->is_open())
        {
            send_message(active_client, msg);
        }
    }

    // Stop and cleanup cameras
    void stop_cameras()
    {
        is_running.store(false);

        if (camera_manager)
        {
            delete camera_manager;
            camera_manager = nullptr;
        }

        if (ball_detection_thread.joinable())
        {
            ball_detection_thread.join();
        }

        if (tag_detection_thread.joinable())
        {
            tag_detection_thread.join();
        }

        if (debug_frame_thread.joinable())
        {
            debug_frame_thread.join();
        }

        // Clean up cameras
        for (auto camera : cameras)
        {
            delete camera;
        }
        cameras.clear();
    }

public:
    TcpServer(boost::asio::io_context &io_context, short port)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        LogDisplay("TCP Server started on port %d", port);
        g_server = this;
        start_accept();
    }

    ~TcpServer()
    {
        stop_cameras();
    }
};

void test_camera()
{
    auto camera = new cv::VideoCapture("/dev/video8", CAP_V4L2);

    if (!camera->isOpened())
    {
        LogErr("Failed to open camera");
        return;
    }

    if (!camera->set(cv::CAP_PROP_FRAME_WIDTH, 640))
        LogErr("Failed to set frame width");
    if (!camera->set(cv::CAP_PROP_FRAME_HEIGHT, 480))
        LogErr("Failed to set frame height");
    if (!camera->set(cv::CAP_PROP_FPS, 206.65))
        LogErr("Failed to set fps");

    LogDisplay("Camera opened at %f fps with resolution %dx%d", camera->get(cv::CAP_PROP_FPS), int(camera->get(cv::CAP_PROP_FRAME_WIDTH)), int(camera->get(cv::CAP_PROP_FRAME_HEIGHT)));

    int64_t last_frame_time = NOW_NS;
    int num_frames = 0;

    while (true)
    {
        cv::Mat frame;
        camera->read(frame);

        if (num_frames >= 200)
        {
            int64_t time_since_last_frame = NOW_NS - last_frame_time;
            LogDisplay("Fps: %f", 1e9 / double(time_since_last_frame) * double(num_frames));
            last_frame_time = NOW_NS;
            num_frames = 0;
        }
        num_frames++;
    }
}

int main()
{
    boost::asio::io_context io_context;

    // Create and start the TCP server on port 8080
    TcpServer server(io_context, 8080);

    LogDisplay("TCP server is running. Press Ctrl+C to stop...");

    // Run the server
    io_context.run();

    return 0;
}
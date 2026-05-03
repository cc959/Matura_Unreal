#pragma once

#include <string>
#include <vector>
#include <utility>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "packer.h"
#include "unpacker.h"
#pragma clang diagnostic pop

#include "GlobalIncludes.h"

namespace TCPMessages
{
	enum _TagFamily
	{
		tag16h5 = 0,
		tag25h9 = 1,
		tag36h11 = 2,
		tagCircle21h7 = 3,
		tagCircle49h12 = 4,
		tagCustom48h12 = 5,
		tagStandard41h12 = 6,
		tagStandard52h13 = 7,
	};

	// Add msgpack serialization for TagFamily
	inline msgpack::packer &operator<<(msgpack::packer &pk, const _TagFamily &family)
	{
		pk << static_cast<int>(family);
		return pk;
	}

	inline msgpack::unpacker &operator>>(msgpack::unpacker &unpk, _TagFamily &family)
	{
		int value;
		unpk >> value;
		family = static_cast<_TagFamily>(value);
		return unpk;
	}

	struct _TagDetection
	{
		int id;
		_TagFamily family;
		double R[9];
		double t[3];
	};

	// Add msgpack serialization for TagDetection
	inline msgpack::packer &operator<<(msgpack::packer &pk, const _TagDetection &detection)
	{
		// Create a map with separate R0-R8 and t0-t2 fields instead of arrays
		pk.map(
			"id", detection.id,
			"family", detection.family,
			"R0", detection.R[0],
			"R1", detection.R[1],
			"R2", detection.R[2],
			"R3", detection.R[3],
			"R4", detection.R[4],
			"R5", detection.R[5],
			"R6", detection.R[6],
			"R7", detection.R[7],
			"R8", detection.R[8],
			"t0", detection.t[0],
			"t1", detection.t[1],
			"t2", detection.t[2]);
		return pk;
	}

	inline msgpack::unpacker &operator>>(msgpack::unpacker &unpk, _TagDetection &detection)
	{
		std::map<std::string, msgpack::unpacker> fields;
		unpk >> fields;

		if (fields.count("id"))
			fields["id"] >> detection.id;
		if (fields.count("family"))
			fields["family"] >> detection.family;

		// Extract R values individually
		if (fields.count("R0"))
			fields["R0"] >> detection.R[0];
		if (fields.count("R1"))
			fields["R1"] >> detection.R[1];
		if (fields.count("R2"))
			fields["R2"] >> detection.R[2];
		if (fields.count("R3"))
			fields["R3"] >> detection.R[3];
		if (fields.count("R4"))
			fields["R4"] >> detection.R[4];
		if (fields.count("R5"))
			fields["R5"] >> detection.R[5];
		if (fields.count("R6"))
			fields["R6"] >> detection.R[6];
		if (fields.count("R7"))
			fields["R7"] >> detection.R[7];
		if (fields.count("R8"))
			fields["R8"] >> detection.R[8];

		// Extract t values individually
		if (fields.count("t0"))
			fields["t0"] >> detection.t[0];
		if (fields.count("t1"))
			fields["t1"] >> detection.t[1];
		if (fields.count("t2"))
			fields["t2"] >> detection.t[2];

		return unpk;
	}

	enum _DetectionType
	{
		BlobDetector = 0,
		ContourFilter = 1,
	};

	// Add msgpack serialization for DetectionType
	inline msgpack::packer &operator<<(msgpack::packer &pk, const _DetectionType &type)
	{
		pk << static_cast<int>(type);
		return pk;
	}

	inline msgpack::unpacker &operator>>(msgpack::unpacker &unpk, _DetectionType &type)
	{
		int value;
		unpk >> value;
		type = static_cast<_DetectionType>(value);
		return unpk;
	}

	enum _DebugFrameType
	{
		None = 0,
		Threshold = 1,
		HueOnly = 2,
		SatOnly = 3,
		ValOnly = 4,
		Preview = 5,
	};

	// Add msgpack serialization for DebugFrameType
	inline msgpack::packer &operator<<(msgpack::packer &pk, const _DebugFrameType &type)
	{
		pk << static_cast<int>(type);
		return pk;
	}

	inline msgpack::unpacker &operator>>(msgpack::unpacker &unpk, _DebugFrameType &type)
	{
		int value;
		unpk >> value;
		type = static_cast<_DebugFrameType>(value);
		return unpk;
	}

	struct _Detection
	{
		std::pair<double, double> position;
		double time;
		int camera_id;
	};

	// Message types for communication
	enum class MessageType
	{
		START_CAMERAS_REQ = 1,
		SET_DEBUG_FRAME_TYPE_REQ = 2,
		UPDATE_CAMERA_PARAMS_REQ = 3,

		TAG_DETECTION_RESP = 101,
		BALL_DETECTION_RESP = 102,
		DEBUG_FRAME_RESP = 103,
		STATUS_RESP = 104
	};

	struct StatusResponse
	{
		bool success;
		std::string message;

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const StatusResponse &resp)
		{
			pk.map("success", resp.success, "message", resp.message);
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, StatusResponse &resp)
		{
			std::map<std::string, msgpack::unpacker> fields;
			unpk >> fields;

			if (fields.count("success"))
				fields["success"] >> resp.success;
			if (fields.count("message"))
				fields["message"] >> resp.message;

			return unpk;
		}
	};

	// Camera parameters structure
	struct _CameraParams
	{
		std::string camera_path;
		std::pair<int, int> resolution = {1280, 720};
		float processing_resolution_factor = 0.5;
		double fps = 60;
		float exposure = 96.0;
		std::pair<double, double> focal_length = {1363.39266, 1363.108124};
		std::pair<double, double> k_twins = {-0.431772, 0.209823}; // distortion coefficients
		std::pair<double, double> p_twins = {-0.006944, 0.007034}; // distortion coefficients
		_DetectionType detection_type = _DetectionType::ContourFilter;
		std::vector<_TagFamily> tag_families = {};
		_DebugFrameType debug_frame_type = _DebugFrameType::Preview;
		bool draw_debug_overlay = true;
		bool debug_output = false;

		// HSV threshold parameters
		int low_H = 40;
		int high_H = 75;
		int low_S = 90;
		int high_S = 255;
		int low_V = 100;
		int high_V = 255;

		// Background subtraction learning rate
		float learning_rate = -1;

		// Minimum blob size for detection
		int min_blob_size = 300;

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const _CameraParams &params)
		{
			// Packing in order - use map to explicitly name fields
			pk.map(
				"camera_path", params.camera_path,
				"resolution_x", params.resolution.first,
				"resolution_y", params.resolution.second,
				"fps", params.fps,
				"processing_resolution_factor", params.processing_resolution_factor,
				"exposure", params.exposure,
				"focal_length_x", params.focal_length.first,
				"focal_length_y", params.focal_length.second,
				"k_twins_x", params.k_twins.first,
				"k_twins_y", params.k_twins.second,
				"p_twins_x", params.p_twins.first,
				"p_twins_y", params.p_twins.second,
				"detection_type", static_cast<int>(params.detection_type),
				"tag_families", params.tag_families,
				"debug_frame_type", static_cast<int>(params.debug_frame_type),
				"draw_debug_overlay", params.draw_debug_overlay,
				"debug_output", params.debug_output,
				"low_H", params.low_H,
				"high_H", params.high_H,
				"low_S", params.low_S,
				"high_S", params.high_S,
				"low_V", params.low_V,
				"high_V", params.high_V,
				"learning_rate", params.learning_rate,
				"min_blob_size", params.min_blob_size);
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, _CameraParams &params)
		{
			// Check if it's a map
			if (unpk.type() != msgpack::unpacker::T_MAP)
			{
				throw msgpack::output_conversion_error("expected map for CameraParams");
			}

			// In practical implementations, we'd need to use the unpacker to browse the map
			// This is a simplified example - in real code we'd need to handle the key/value pairs
			std::map<std::string, msgpack::unpacker> fields;
			unpk >> fields;

			// Extract fields by name
			if (fields.count("camera_path"))
				fields["camera_path"] >> params.camera_path;

			// Handle pairs as separate x/y components
			if (fields.count("resolution_x"))
				fields["resolution_x"] >> params.resolution.first;
			if (fields.count("resolution_y"))
				fields["resolution_y"] >> params.resolution.second;

			if (fields.count("fps"))
				fields["fps"] >> params.fps;

			if (fields.count("processing_resolution_factor"))
				fields["processing_resolution_factor"] >> params.processing_resolution_factor;
			if (fields.count("exposure"))
				fields["exposure"] >> params.exposure;

			if (fields.count("focal_length_x"))
				fields["focal_length_x"] >> params.focal_length.first;
			if (fields.count("focal_length_y"))
				fields["focal_length_y"] >> params.focal_length.second;

			if (fields.count("k_twins_x"))
				fields["k_twins_x"] >> params.k_twins.first;
			if (fields.count("k_twins_y"))
				fields["k_twins_y"] >> params.k_twins.second;

			if (fields.count("p_twins_x"))
				fields["p_twins_x"] >> params.p_twins.first;
			if (fields.count("p_twins_y"))
				fields["p_twins_y"] >> params.p_twins.second;

			// For enums, we need to cast from int
			if (fields.count("detection_type"))
			{
				int tmp;
				fields["detection_type"] >> tmp;
				params.detection_type = static_cast<_DetectionType>(tmp);
			}

			if (fields.count("tag_families"))
				fields["tag_families"] >> params.tag_families;

			if (fields.count("debug_frame_type"))
			{
				int tmp;
				fields["debug_frame_type"] >> tmp;
				params.debug_frame_type = static_cast<_DebugFrameType>(tmp);
			}

			if (fields.count("draw_debug_overlay"))
				fields["draw_debug_overlay"] >> params.draw_debug_overlay;
			if (fields.count("debug_output"))
				fields["debug_output"] >> params.debug_output;
			if (fields.count("low_H"))
				fields["low_H"] >> params.low_H;
			if (fields.count("high_H"))
				fields["high_H"] >> params.high_H;
			if (fields.count("low_S"))
				fields["low_S"] >> params.low_S;
			if (fields.count("high_S"))
				fields["high_S"] >> params.high_S;
			if (fields.count("low_V"))
				fields["low_V"] >> params.low_V;
			if (fields.count("high_V"))
				fields["high_V"] >> params.high_V;
			if (fields.count("learning_rate"))
				fields["learning_rate"] >> params.learning_rate;
			if (fields.count("min_blob_size"))
				fields["min_blob_size"] >> params.min_blob_size;

			return unpk;
		}
	};

	// Start cameras request
	struct StartCamerasRequest
	{
		std::vector<_CameraParams> camera_params;

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const StartCamerasRequest &req)
		{
			pk.map("camera_params", req.camera_params);
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, StartCamerasRequest &req)
		{
			std::map<std::string, msgpack::unpacker> fields;
			unpk >> fields;

			if (fields.count("camera_params"))
			{
				fields["camera_params"] >> req.camera_params;
			}

			return unpk;
		}
	};

	// Update camera parameters request (without changing path and resolution)
	struct UpdateCameraParamsRequest
	{
		int camera_id;
		_CameraParams params;

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const UpdateCameraParamsRequest &req)
		{
			pk.map("camera_id", req.camera_id, "params", req.params);
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, UpdateCameraParamsRequest &req)
		{
			std::map<std::string, msgpack::unpacker> fields;
			unpk >> fields;

			if (fields.count("camera_id"))
				fields["camera_id"] >> req.camera_id;
			if (fields.count("params"))
				fields["params"] >> req.params;

			return unpk;
		}
	};

	// Set debug frame type request
	struct SetDebugFrameTypeRequest
	{
		int camera_id;
		_DebugFrameType debug_frame_type;

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const SetDebugFrameTypeRequest &req)
		{
			pk.map(
				"camera_id", req.camera_id,
				"debug_frame_type", static_cast<int>(req.debug_frame_type));
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, SetDebugFrameTypeRequest &req)
		{
			std::map<std::string, msgpack::unpacker> fields;
			unpk >> fields;

			if (fields.count("camera_id"))
				fields["camera_id"] >> req.camera_id;

			if (fields.count("debug_frame_type"))
			{
				int tmp;
				fields["debug_frame_type"] >> tmp;
				req.debug_frame_type = static_cast<_DebugFrameType>(tmp);
			}

			return unpk;
		}
	};

	// Ball detection response
	struct BallDetectionResponse
	{
		int camera_id;
		std::pair<double, double> position;
		double timestamp;

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const BallDetectionResponse &resp)
		{
			pk.map(
				"camera_id", resp.camera_id,
				"position_x", resp.position.first,
				"position_y", resp.position.second,
				"timestamp", resp.timestamp);
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, BallDetectionResponse &resp)
		{
			std::map<std::string, msgpack::unpacker> fields;
			unpk >> fields;

			if (fields.count("camera_id"))
				fields["camera_id"] >> resp.camera_id;
			if (fields.count("position_x"))
				fields["position_x"] >> resp.position.first;
			if (fields.count("position_y"))
				fields["position_y"] >> resp.position.second;
			if (fields.count("timestamp"))
				fields["timestamp"] >> resp.timestamp;

			return unpk;
		}
	};

	// Tag detection response
	struct TagDetectionResponse
	{
		int camera_id;
		std::vector<_TagDetection> detections;

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const TagDetectionResponse &resp)
		{
			pk.map("camera_id", resp.camera_id, "detections", resp.detections);
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, TagDetectionResponse &resp)
		{
			std::map<std::string, msgpack::unpacker> fields;
			unpk >> fields;

			if (fields.count("camera_id"))
				fields["camera_id"] >> resp.camera_id;
			if (fields.count("detections"))
				fields["detections"] >> resp.detections;

			return unpk;
		}
	};

	// Debug frame response
	struct DebugFrameResponse
	{
		int camera_id;
		std::string frame_data;
		int width;
		int height;
		int type;

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const DebugFrameResponse &resp)
		{
			pk.map(
				"camera_id", resp.camera_id,
				"frame_data", resp.frame_data,
				"width", resp.width,
				"height", resp.height,
				"type", resp.type);
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, DebugFrameResponse &resp)
		{
			std::map<std::string, msgpack::unpacker> fields;
			unpk >> fields;

			if (fields.count("camera_id"))
				fields["camera_id"] >> resp.camera_id;
			if (fields.count("frame_data"))
				fields["frame_data"] >> resp.frame_data;
			if (fields.count("width"))
				fields["width"] >> resp.width;
			if (fields.count("height"))
				fields["height"] >> resp.height;
			if (fields.count("type"))
				fields["type"] >> resp.type;

			return unpk;
		}
	};

	// Generic container for all message types
	struct Message
	{
		MessageType type;
		std::string data;

		template <typename T>
		static Message create(MessageType type, const T &content)
		{
			Message msg;
			msg.type = type;

			// Use msgpack packer to serialize content
			msgpack::packer pk;
			pk << content;

			// Get the buffer with serialized data
			auto data = pk.get_buffer();
			msg.data = std::string(data.begin(), data.end());

			return msg;
		}

		template <typename T>
		void parse(T &content) const
		{
			// Use msgpack unpacker to deserialize content
			msgpack::unpacker unpk(std::vector<uint8_t>(data.begin(), data.end()));
			unpk >> content;
		}

		// Serialize using msgpack
		friend msgpack::packer &operator<<(msgpack::packer &pk, const Message &msg)
		{
			auto temp = static_cast<int>(msg.type);
			pk << temp << msg.data;
			return pk;
		}

		// Deserialize using msgpack
		friend msgpack::unpacker &operator>>(msgpack::unpacker &unpk, Message &msg)
		{
			int temp;
			unpk >> temp >> msg.data;
			msg.type = static_cast<MessageType>(temp);

			return unpk;
		}
	};

} // namespace TCPMessages
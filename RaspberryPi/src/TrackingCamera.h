// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <map>
#include <queue>
#include <utility>
#include <string>
#include <vector>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/video.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgcodecs.hpp"
#include <opencv2/bgsegm.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp> // For std::pair
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "packer.h"
#include "unpacker.h"

#include "apriltag/apriltag.h"
#include "apriltag/tag16h5.h"
#include "apriltag/tag25h9.h"
#include "apriltag/tag36h11.h"
#include "apriltag/tagCircle21h7.h"
#include "apriltag/tagCircle49h12.h"
#include "apriltag/tagCustom48h12.h"
#include "apriltag/tagStandard41h12.h"
#include "apriltag/tagStandard52h13.h"
#include "apriltag/apriltag_pose.h"

#include "TCPMessages.h"

using namespace cv;
using namespace std;
using namespace TCPMessages;

class ATrackingCamera
{

public:
	// Sets default values for this actor's properties
	ATrackingCamera(string path, vector<_TagFamily> tag_families);
	~ATrackingCamera();

	void InitCamera();
	void CreateTagDetector();

	double SyncFrame();
	void GetFrame();
	Point2d FindBall();
	vector<_TagDetection> UpdateTags(Mat frame);
	void ReleaseTagDetector();

	void ReleaseCamera();

	Size cv_size;

	Mat K() const;
	Mat p() const;
	Mutex destroy_lock;
	bool loaded = false;
	bool in_use = false;

	Point2d ball = {-1, -1};
	Point2d used_ball = {-1, -1};

	int64_t next_update_time = 0;

	Mat cv_frame;

	double last_frame_time;

	// the camera manager want to update tags, can't as is in other thread
	bool must_update_tags = false;

protected:
	VideoCapture cv_cap;

	Ptr<BackgroundSubtractor> cv_bg_subtractor;
	Ptr<SimpleBlobDetector> cv_blob_detector;
	apriltag_detector *at_td = nullptr;
	vector<apriltag_family_t *> created_families;

	Mat cv_undistort_map1, cv_undistort_map2;

	std::vector<Point2f> ball_path;
	int ball_steps_skipped = 0;

	Mutex last_tags_mut;
	std::vector<apriltag_detection_t> last_tags;

public:
	Mat cv_debug_frame;

	string camera_path;

	vector<_TagFamily> tag_families;

	pair<int, int> resolution = {1280, 720};
	double fps = 60;

	float processing_resolution_factor = 0.5;

	float exposure = 127.0;

	pair<double, double> focal_length = {1363.39266, 1363.108124};

	pair<double, double> k_twins = {-0.431772, 0.209823};

	pair<double, double> p_twins = {-0.006944, 0.007034};

	int low_H = 40;
	int low_S = 90;
	int low_V = 100;
	int high_H = 75;
	int high_S = 255;
	int high_V = 255;

	bool debug_output = false;

	float learning_rate = -1;

	int min_blob_size = 300;

	_DebugFrameType debug_frame_type = None;

	bool apply_threshold_to_debug_frame = false;

	bool draw_debug_overlay = true;

	_DetectionType detection_type = _DetectionType::ContourFilter;
};

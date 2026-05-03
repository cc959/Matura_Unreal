// Fill out your copyright notice in the Description page of Project Settings.

#include "TrackingCamera.h"

#include <string>
#include <functional>
#include <set>
#include <map>
#include <thread>

#include "GlobalIncludes.h"
#include "TCPMessages.h"

using namespace TCPMessages;

ATrackingCamera::ATrackingCamera(string path, vector<_TagFamily> tag_families)
{
	camera_path = path;
	this->tag_families = tag_families;
}

void ATrackingCamera::InitCamera()
{
	loaded = false;

	LogDisplay("Initializing camera at path: %s", camera_path.c_str());

	if (camera_path != "")
		cv_cap.open(camera_path, CAP_V4L2);
	else
		LogErr(TEXT("Invalid camera path!"));

	if (cv_cap.isOpened())
	{
		LogDisplay("Camera %s opened", camera_path.c_str());
		// if (!cv_cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G')))
		//	LogWarning(TEXT("Could not set MJPG format"));

		// if (!cv_cap.set(CAP_PROP_CONVERT_RGB, 0)) // UE OpenCV plugin broken, can't decompress jpeg, must do manually
		// 	LogWarning(TEXT("Could not enable raw output format"));

		if (!cv_cap.set(CAP_PROP_FRAME_WIDTH, resolution.first) || !cv_cap.set(
																	   CAP_PROP_FRAME_HEIGHT, resolution.second))
			LogWarning(TEXT("Could not set size"));

		if (!cv_cap.set(CAP_PROP_FPS, fps))
			LogWarning(TEXT("Could not set fps"));

		if (!cv_cap.set(CAP_PROP_AUTOFOCUS, 0))
			LogWarning(TEXT("Could not set autofocus"));

		if (!cv_cap.set(CAP_PROP_FOCUS, 0))
			LogWarning(TEXT("Could not set focus"));

		if (!cv_cap.set(CAP_PROP_AUTO_EXPOSURE, 1)) // this means no auto exposure, ¯\_(ツ)_/¯
			LogWarning(TEXT("Could not turn off auto exposure"));

		if (!cv_cap.set(CAP_PROP_EXPOSURE, exposure))
			LogWarning(TEXT("Could not set exposure to %f"), exposure);

		cv_size = Size(cv_cap.get(CAP_PROP_FRAME_WIDTH), cv_cap.get(CAP_PROP_FRAME_HEIGHT));

		LogWarning(TEXT("Opened camera at %s with %dx%d at %f fps"), camera_path.c_str(),
				   int(cv_cap.get(CAP_PROP_FRAME_WIDTH)), int(cv_cap.get(CAP_PROP_FRAME_HEIGHT)),
				   cv_cap.get(CAP_PROP_FPS));

		LogWarning(TEXT("Requested camera had %dx%d at %f fps"), resolution.first, resolution.second, fps);

		LogDisplay(TEXT("Camera GUID: %d"), int(cv_cap.get(CAP_PROP_GUID)));
	}
	else
	{
		LogErr(TEXT("Could not open camera at path: %s"), camera_path.c_str());
		return;
	}

	initUndistortRectifyMap(K(), p(), {}, {}, cv_size, CV_32FC1, cv_undistort_map1,
							cv_undistort_map2);

	cv_bg_subtractor = createBackgroundSubtractorMOG2();

	SimpleBlobDetector::Params cv_blob_params;
	// memset(&cv_blob_params, 0, sizeof(SimpleBlobDetector::Params));

	// cv_blob_params.filterByColor = true;
	cv_blob_params.blobColor = 255;

	cv_blob_params.filterByConvexity = true;
	cv_blob_params.minConvexity = 0.6;
	cv_blob_params.maxConvexity = 1;

	cv_blob_params.filterByArea = true;
	cv_blob_params.minArea = 50;
	cv_blob_params.maxArea = 50000;

	cv_blob_params.filterByCircularity = false;
	cv_blob_params.minCircularity = 0.6;
	cv_blob_params.maxCircularity = 1;

	LogDisplay("Blob detector params - thresholdStep: %f, minThreshold: %f, maxThreshold: %f, minRepeatability: %zu, minDistBetweenBlobs: %f, filterByColor: %d, blobColor: %d, filterByArea: %d, minArea: %f, maxArea: %f, filterByCircularity: %d, minCircularity: %f, maxCircularity: %f, filterByInertia: %d, minInertiaRatio: %f, maxInertiaRatio: %f, filterByConvexity: %d, minConvexity: %f, maxConvexity: %f",
			   cv_blob_params.thresholdStep, cv_blob_params.minThreshold, cv_blob_params.maxThreshold,
			   cv_blob_params.minRepeatability, cv_blob_params.minDistBetweenBlobs,
			   cv_blob_params.filterByColor, cv_blob_params.blobColor,
			   cv_blob_params.filterByArea, cv_blob_params.minArea, cv_blob_params.maxArea,
			   cv_blob_params.filterByCircularity, cv_blob_params.minCircularity, cv_blob_params.maxCircularity,
			   cv_blob_params.filterByInertia, cv_blob_params.minInertiaRatio, cv_blob_params.maxInertiaRatio,
			   cv_blob_params.filterByConvexity, cv_blob_params.minConvexity, cv_blob_params.maxConvexity);

	cv_blob_detector = SimpleBlobDetector::create(cv_blob_params);

	LogDisplay("Camera %s initialized", camera_path.c_str());

	CreateTagDetector();
}

void ATrackingCamera::CreateTagDetector()
{
	if (at_td)
		return;

	at_td = apriltag_detector_create();

	at_td->quad_decimate = 1.0; // decimate factor
	at_td->quad_sigma = 0.0;	// apply this much low-pass blur to input
	at_td->nthreads = 8;		// use this many cpu threads
	at_td->debug = false;		// print debug output
	at_td->refine_edges = true; // refine tag edges

	static const std::function<apriltag_family_t *()> family_functions[] = {
		tag16h5_create, tag25h9_create, tag36h11_create, tagCircle21h7_create, tagCircle49h12_create,
		tagCustom48h12_create, tagStandard41h12_create, tagStandard52h13_create};

	for (_TagFamily family : tag_families)
	{
		apriltag_family_t *fam = family_functions[family]();
		apriltag_detector_add_family(at_td, fam);
		created_families.push_back(fam);
	}

	loaded = true;
}

Mat ATrackingCamera::K() const
{
	return (cv::Mat_<double>(3, 3) << focal_length.first, 0, cv_size.width / 2,
			0, focal_length.second, cv_size.height / 2,
			0, 0, 1);
}

Mat ATrackingCamera::p() const
{
	return (cv::Mat_<double>(5, 1) << k_twins.first, k_twins.second, p_twins.first, p_twins.second, 0);
}

double ATrackingCamera::SyncFrame()
{
	if (!cv_cap.isOpened() || !loaded)
		return 0;

	cv_cap.grab();
	double time_captured = cv_cap.get(CAP_PROP_POS_MSEC);

	if (debug_output)
		LogDisplay(TEXT("Camera %s grabbed frame at %f ms"), camera_path.c_str(), time_captured);

	return time_captured;
}

void ATrackingCamera::GetFrame()
{
	if (!cv_cap.isOpened() || !loaded || camera_path == "")
		return;

	Mat cv_frame_distorted;
	cv_cap.read(cv_frame);

	// Mat frame_bgr;

	// remap(cv_frame_distorted, cv_frame, cv_undistort_map1, cv_undistort_map2, INTER_LINEAR);

	// cvtColor(frame_bgr, cv_frame, COLOR_BGR2RGB);

	if (cv_frame.empty())
	{
		LogWarning(TEXT("Frame is empty after decompression"));
		return;
	}
}

Point2d ATrackingCamera::FindBall()
{
	_DebugFrameType debug_frame_type_temp = debug_frame_type;

	if (!cv_cap.isOpened() || !loaded)
		return {};

	if (cv_frame.empty())
	{
		LogWarning(TEXT("cv_frame is empty, cannot find ball"));
		return {};
	}

	auto time_before = std::chrono::high_resolution_clock::now();

	Mat cv_frame_scaled;
	float factor_used = processing_resolution_factor;
	resize(cv_frame, cv_frame_scaled, Size(), factor_used, factor_used, INTER_AREA);

	Mat cv_frame_HSV, cv_color_threshold, cv_bg_threshold, cv_threshold;
	cvtColor(cv_frame_scaled, cv_frame_HSV, COLOR_RGB2HSV);

	if (debug_frame_type_temp != TCPMessages::None && debug_frame_type_temp != TCPMessages::Threshold && apply_threshold_to_debug_frame)
	{
		if (debug_frame_type_temp == TCPMessages::HueOnly)
			inRange(cv_frame_HSV, Scalar(low_H, 0, 0), Scalar(high_H, 255, 255), cv_color_threshold);
		if (debug_frame_type_temp == TCPMessages::SatOnly)
			inRange(cv_frame_HSV, Scalar(0, low_S, 0), Scalar(180, high_S, 255), cv_color_threshold);
		if (debug_frame_type_temp == TCPMessages::ValOnly)
			inRange(cv_frame_HSV, Scalar(0, 0, low_V), Scalar(180, 255, high_V), cv_color_threshold);
	}
	else
	{
		inRange(cv_frame_HSV, Scalar(low_H, low_S, low_V), Scalar(high_H, high_S, high_V), cv_color_threshold);
	}

	if (learning_rate != 1) // used to "deactivate" the background subtraction
	{
		cv_bg_subtractor->apply(cv_frame_scaled, cv_bg_threshold, learning_rate);
		bitwise_and(cv_color_threshold, cv_bg_threshold, cv_threshold);
	}
	else
	{
		swap(cv_threshold, cv_color_threshold);
	}

	if (cv_threshold.empty())
	{
		LogWarning(TEXT("threshold frame is empty!"));
		return {};
	}

	Point2f det = {-1, -1};

	Mat cv_debug_frame_temp;

	if (debug_frame_type_temp != None)
	{

		if (debug_frame_type_temp == Threshold)
		{
			resize(cv_threshold, cv_debug_frame_temp, {}, 1 / factor_used, 1 / factor_used);
			cvtColor(cv_debug_frame_temp, cv_debug_frame_temp, COLOR_GRAY2RGB);
		}
		else if (debug_frame_type_temp == Preview)
		{
			if (apply_threshold_to_debug_frame)
			{
				Mat cv_resized_threshold;
				resize(cv_threshold, cv_resized_threshold, {}, 1 / factor_used, 1 / factor_used);
				cv_frame.copyTo(cv_debug_frame_temp, cv_resized_threshold);
			}
			else
			{
				cv_debug_frame_temp = cv_frame.clone();
			}
		}
		else
		{
			auto setChannel = [](Mat &mat, unsigned int channel, unsigned char value)
			// https://stackoverflow.com/questions/23510571/how-to-set-given-channel-of-a-cvmat-to-a-given-value-efficiently-without-chang
			{
				// make sure have enough channels
				if (mat.channels() < int(channel + 1))
					return;

				const int cols = mat.cols;
				const int step = mat.channels();
				const int rows = mat.rows;
				for (int y = 0; y < rows; y++)
				{
					// get pointer to the first byte to be changed in this row
					unsigned char *p_row = mat.ptr(y) + channel;
					unsigned char *row_end = p_row + cols * step;
					for (; p_row != row_end; p_row += step)
						*p_row = value;
				}
			};

			Mat cv_frame_HSV_resized;
			resize(cv_frame_HSV, cv_frame_HSV_resized, {}, 1 / factor_used, 1 / factor_used);

			Mat sv_channels(cv_frame_HSV_resized.size(), cv_frame_HSV_resized.type(), Scalar(255));

			if (debug_frame_type_temp == HueOnly)
			{
				setChannel(cv_frame_HSV_resized, 1, 255);
				setChannel(cv_frame_HSV_resized, 2, 255);
			}
			if (debug_frame_type_temp == SatOnly)
			{
				setChannel(cv_frame_HSV_resized, 2, 255);
				setChannel(cv_frame_HSV_resized, 0, 255);
			}
			if (debug_frame_type_temp == ValOnly)
			{
				setChannel(cv_frame_HSV_resized, 0, 255);
				setChannel(cv_frame_HSV_resized, 1, 0);
			}

			cvtColor(cv_frame_HSV_resized, cv_frame_HSV_resized, COLOR_HSV2RGB);

			if (apply_threshold_to_debug_frame)
			{
				Mat cv_resized_threshold;
				resize(cv_threshold, cv_resized_threshold, {}, 1 / factor_used, 1 / factor_used);
				cv_frame_HSV_resized.copyTo(cv_debug_frame_temp, cv_resized_threshold);
			}
			else
			{
				cv_debug_frame_temp = cv_frame_HSV_resized.clone();
			}
		}
	}

	if (detection_type == BlobDetector)
	{
		std::vector<KeyPoint> points;
		cv_blob_detector->detect(cv_threshold, points);

		sort(points.begin(), points.end(), [](const KeyPoint &a, const KeyPoint &b)
			 { return a.size > b.size; });

		if (points.size())
		{
			det = points[0].pt / factor_used;

			const int radius = 50;
			if (draw_debug_overlay && debug_frame_type_temp != None)
			{

				circle(cv_debug_frame_temp, det, radius, Scalar(255, 0, 0), 3);
				line(cv_debug_frame_temp, det - Point2f(radius, 0), det + Point2f(radius, 0),
					 Scalar(255, 0, 0), 3);
				line(cv_debug_frame_temp, det - Point2f(0, radius), det + Point2f(0, radius),
					 Scalar(255, 0, 0), 3);
			}
		}
	}
	else if (detection_type == ContourFilter)
	{
		// Find contours in the image
		std::vector<std::vector<Point>> contours;
		findContours(cv_threshold, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		int best_contour = -1;
		double area = -1;
		for (int i = 0; i < contours.size(); i++)
		{
			double c_area = contourArea(contours[i]);
			if (c_area > area)
			{
				area = c_area;
				best_contour = i;
			}
		}

		if (best_contour != -1 && area > min_blob_size * factor_used * factor_used) // at least 20x20 pixels seems reasonable
		{
			std::vector<std::vector<Point>> contours_to_draw = {{}};

			for (Point p : contours[best_contour])
				contours_to_draw[0].push_back(p / factor_used);

			if (draw_debug_overlay && debug_frame_type_temp != None)
				drawContours(cv_debug_frame_temp, contours_to_draw, -1, cv::Scalar(0, 0, 255), 2);

			RotatedRect bounding_box = minAreaRect(contours[best_contour]);
			Point2f points[4];
			bounding_box.points(points);

			if (draw_debug_overlay && debug_frame_type_temp != None)
				for (int i = 0; i < 4; i++)
					line(cv_debug_frame_temp, points[i] / factor_used, points[(i + 1) % 4] / factor_used, Scalar(255, 0, 0), 3);

			det = bounding_box.center / factor_used;
		}
	}

	if (det.x != -1 && det.y != -1)
	{
		ball_steps_skipped = 0;
		ball_path.push_back(det);

		if (debug_output)
			LogWarning(TEXT("Camera %s detected ball at: %f %f!"), camera_path.c_str(), det.x, det.y);
	}
	else if (ball_steps_skipped++ == 5)
		ball_path.clear();

	if (draw_debug_overlay && debug_frame_type_temp != None)
		for (int i = 0; i < int(ball_path.size()) - 1; i++)
		{
			line(cv_debug_frame_temp, ball_path[i], ball_path[i + 1], Scalar(0, 255, 0), 3);
		}

	auto time_after = std::chrono::high_resolution_clock::now();

	if (debug_output)
		LogDisplay(TEXT("Took camera %s %f ms to find ball"), camera_path.c_str(), (time_after - time_before).count() / 1e6);

	if (draw_debug_overlay && debug_frame_type_temp != None)
	{
		last_tags_mut.lock();
		for (auto det : last_tags)
		{
			line(cv_debug_frame_temp, Point(det.p[0][0], det.p[0][1]),
				 Point(det.p[1][0], det.p[1][1]),
				 Scalar(0xff, 0, 0, 0xff), 2);
			line(cv_debug_frame_temp, Point(det.p[0][0], det.p[0][1]),
				 Point(det.p[3][0], det.p[3][1]),
				 Scalar(0, 0xff, 0, 0xff), 2);
			line(cv_debug_frame_temp, Point(det.p[1][0], det.p[1][1]),
				 Point(det.p[2][0], det.p[2][1]),
				 Scalar(0, 0, 0xff, 0xff), 2);
			line(cv_debug_frame_temp, Point(det.p[2][0], det.p[2][1]),
				 Point(det.p[3][0], det.p[3][1]),
				 Scalar(0, 0, 0xff, 0xff), 2);

			std::string text = std::to_string(det.id);
			int fontface = FONT_HERSHEY_SCRIPT_SIMPLEX;
			double fontscale = 1.0;
			int baseline;
			Size textsize = getTextSize(text, fontface, fontscale, 2,
										&baseline);
			putText(cv_debug_frame_temp, text, Point(det.c[0] - textsize.width / 2, det.c[1] + textsize.height / 2),
					fontface, fontscale, Scalar(0, 0x99, 0xff, 0xff), 2);
		}
		last_tags_mut.unlock();
	}

	if (debug_frame_type_temp != None)
		cvtColor(cv_debug_frame_temp, cv_debug_frame, COLOR_RGB2RGBA);

	return ball = det;
}

vector<_TagDetection> ATrackingCamera::UpdateTags(Mat frame)
{
	if (frame.empty())
	{
		LogWarning(TEXT("cv_frame is empty, cannot localize camera"));
		return {};
	}

	if (!at_td)
	{
		LogWarning(TEXT("Tag detector is null, cannot detect"));
		return {};
	}

	auto time_before = std::chrono::high_resolution_clock::now();

	Mat cv_frame_gray;

	cvtColor(frame, cv_frame_gray, COLOR_RGB2GRAY);

	// Make an image_u8_t header for the Mat data
	image_u8_t im{
		cv_frame_gray.cols,
		cv_frame_gray.rows,
		cv_frame_gray.cols,
		cv_frame_gray.data};

	if (cv_frame_gray.empty())
	{
		LogWarning(TEXT("cv_frame is empty, cannot localize camera"));
		return {};
	}

	zarray_t *detections = apriltag_detector_detect(at_td, &im);

	last_tags_mut.lock();
	last_tags.clear();

	vector<_TagDetection> tag_detections;

	// Draw detection outlines
	for (int i = 0; i < zarray_size(detections); i++)
	{
		static std::map<std::string, _TagFamily> family_names =
			{{"tag16h5", _TagFamily::tag16h5},
			 {"tag25h9", _TagFamily::tag25h9},
			 {"tag36h11", _TagFamily::tag36h11},
			 {"tagCircle21h7", _TagFamily::tagCircle21h7},
			 {"tagCircle49h12", _TagFamily::tagCircle49h12},
			 {"tagCustom48h12", _TagFamily::tagCustom48h12},
			 {"tagStandard41h12", _TagFamily::tagStandard41h12},
			 {"tagStandard52h13", _TagFamily::tagStandard52h13}};

		apriltag_detection_t *det;
		zarray_get(detections, i, &det);

		last_tags.push_back(*det);

		std::string name = det->family->name;

		apriltag_detection_info_t info;
		info.det = det;
		info.tagsize = 1;
		info.fx = focal_length.first;
		info.fy = focal_length.second;
		info.cx = cv_size.width / 2;
		info.cy = cv_size.height / 2;

		apriltag_pose_t pose;
		estimate_tag_pose(&info, &pose);

		_TagDetection detection;
		detection.id = det->id;
		detection.family = family_names[name];
		detection.R[0] = pose.R->data[0];
		detection.R[1] = pose.R->data[1];
		detection.R[2] = pose.R->data[2];
		detection.R[3] = pose.R->data[3];
		detection.R[4] = pose.R->data[4];
		detection.R[5] = pose.R->data[5];
		detection.R[6] = pose.R->data[6];
		detection.R[7] = pose.R->data[7];
		detection.R[8] = pose.R->data[8];
		detection.t[0] = pose.t->data[0];
		detection.t[1] = pose.t->data[1];
		detection.t[2] = pose.t->data[2];

		tag_detections.push_back(detection);
	}

	last_tags_mut.unlock();

	apriltag_detections_destroy(detections);

	auto time_after = std::chrono::high_resolution_clock::now();

	if (debug_output)
		LogDisplay(TEXT("Took camera %s %f ms to find apriltag"), camera_path.c_str(), (time_after - time_before).count() / 1e6);

	return tag_detections;
}

void ATrackingCamera::ReleaseTagDetector()
{
	if (at_td)
	{
		apriltag_detector_destroy(at_td);
	}

	// destroy tag families
	for (apriltag_family_t *tf : created_families)
	{
		if (!tf)
		{
			LogWarning(TEXT("Could not destroy tag family"));
			continue;
		}

		LogDisplay("Deleted %s", tf->name);

		if (!strcmp(tf->name, "tag36h11"))
		{
			tag36h11_destroy(tf);
		}
		else if (!strcmp(tf->name, "tag25h9"))
		{
			tag25h9_destroy(tf);
		}
		else if (!strcmp(tf->name, "tag16h5"))
		{
			tag16h5_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagCircle21h7"))
		{
			tagCircle21h7_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagCircle49h12"))
		{
			tagCircle49h12_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagStandard41h12"))
		{
			tagStandard41h12_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagStandard52h13"))
		{
			tagStandard52h13_destroy(tf);
		}
		else if (!strcmp(tf->name, "tagCustom48h12"))
		{
			tagCustom48h12_destroy(tf);
		}
	}

	created_families.clear();

	at_td = nullptr;
}

void ATrackingCamera::ReleaseCamera()
{
	loaded = false;
	destroy_lock.lock();

	cv_cap.release();

	ReleaseTagDetector();
}

ATrackingCamera::~ATrackingCamera()
{
	LogWarning(TEXT("%s is being destroyed"), camera_path.c_str());
	ReleaseCamera();
	LogWarning(TEXT("%s is done being destroyed"), camera_path.c_str());
}

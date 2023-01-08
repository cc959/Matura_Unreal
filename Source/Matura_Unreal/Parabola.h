// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "PreOpenCVHeaders.h"
#include "opencv2/core.hpp"
#include "opencv2/tracking.hpp"
#include "PostOpenCVHeaders.h"

using namespace cv;

class MATURA_UNREAL_API Parabola
{
public:
	struct Prediction
	{
		Mat x, P;
	};
private:
	/// state-transition model
	static Mat F(double dt);

	/// control input
	static Mat Bu(double dt);

	/// observation covariance
	static Mat R(double s1, double d2);

	/// physical model covariance (ie. drag)
	static Mat Q();

	/// time of last update [s]
	double last_t;
	/// last state estimate [m, m, m, m/s, m/s, m/s]
	Prediction last_est;
	
public:
	
	/**
	 * @brief Initialize the Kalman Filter with triangulated positions
	 * @param x initial position and velocity
	 * @param P initial estimate covariance
	 * @param t time of initial variables
	 */
	Parabola(Mat x, Mat P, double t);

	Prediction Predict(double t);
	
	/**
	 * @brief 
	 * @param P Camera to World Projection Matrix
	 * @param feature_point 2D Point in the Image
	 */
	void Update(Mat P, Point2i feature_point, double t);
	
	~Parabola();
};

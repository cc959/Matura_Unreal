// Fill out your copyright notice in the Description page of Project Settings.


#include "Parabola.h"


Mat Parabola::F(double dt)
{
	return Mat_<double>(6, 6) <<
		1, 0, 0, dt, 0, 0,
		0, 1, 0, 0, dt, 0,
		0, 0, 1, 0, 0, dt,
		0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 1;
}

Mat Parabola::Bu(double dt)
{
	return (Mat_<double>(6, 1) << 0, 0, 0.5 * dt * dt, 0, 0, dt) * -9.806;
}

Mat Parabola::R(double s1, double s2)
{
	return Mat_<double>(2, 2) <<
		s1 * s1, 0,
		0, s2 * s2;
}

Mat Parabola::Q()
{
	return Mat_<double>(6, 6) << // TODO: Add drag perhaps?
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0;
}

Parabola::Parabola(Mat x, Mat P, double t) : last_t{t}, last_est{x, P}
{}

Parabola::Prediction Parabola::Predict(double t)
{
	return {F(t - last_t) * last_est.x + Bu(t - last_t), F(t - last_t) * last_est.P * F(t - last_t).t() + Q()};
}

void Parabola::Update(Mat P, Point2i feature_point, double t)
{
	assert(t > this->t);

	Prediction oldk = Predict(t);
	
	Mat lx = (Mat_<double>(3, 1) << -1, 0, feature_point.x);
	Mat ly = (Mat_<double>(3, 1) << 0, -1, feature_point.y);

	Mat πx = P.t() * lx;
	Mat πy =  P.t() * ly;

	// normalize the first three elements of the plane
	πx /= sqrt(πx.rowRange(0, 3).dot(πx.rowRange(0, 3)));
	πy /= sqrt(πy.rowRange(0, 3).dot(πy.rowRange(0, 3)));

	double w1 = πx.at<double>(3), w2 = πy.at<double>(3);

	Mat H;
	vconcat(πx.rowRange(0, 3).t(), πy.rowRange(0, 3).t(), H);

	Mat z = (Mat_<double>(2, 1) << w1, w2);
	Mat y = z - H * oldk.x;

	Mat S = H * oldk.P * H.t() + R(w1, w2);
	Mat K = oldk.P * H.t() * S.inv();

	Prediction kk = {oldk.x + K * y, (Mat::eye(6, 6, CV_64F) - K*H) * oldk.P};
	
	last_est = kk;
	last_t = t;
}

Parabola::~Parabola()
{
}

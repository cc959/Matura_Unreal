// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include <deque>

#include "Ball.h"
#include "MatrixTypes.h"
#include "TrackingCamera.h"


class MATURA_UNREAL_API CameraManager : public FRunnable
{
public:

	// Constructor, create the thread by calling this
	CameraManager(class ABall* ball);

	// Destructor
	virtual ~CameraManager() override;


	// Overriden from FRunnable
	// Do not call these functions youself, that will happen automatically
	// Do your setup here, allocate memory, ect.
	virtual bool Init() override;
	// Main data processing happens here
	virtual uint32 Run() override;
	// Clean up any memory you allocated here
	virtual void Stop() override;

	void DrawBallHistory();
	
private:

	struct Detection
	{
		Point2d position;
		double time;
	};

	struct Position
	{
		FVector position;
		double time;
	};

	std::mutex ball_position_mut;
	std::deque<Position> ball_positions;

	struct ParabPath {
		double a = 0, b = 0, c = 0;
		double px, py;
		double vx, vy;
		double t0, t1;
		
		ParabPath(double a, double b, double c, double px, double py, double vx, double vy, double t0, double t1) : a(a), b(b), c(c), px(px), py(py), vx(vx), vy(vy), t0(t0), t1(t1) {}

		ParabPath() {}
        
		static ParabPath from2Points(Position p1, Position p2, double a) {

			double t0 = p1.time;
			double t1 = p2.time;

			p1.time -= t0;
			p2.time -= t0;
			
			TMatrix2<double> A;
			A.Row0 = {p1.time, 1};
			A.Row1 = {p2.time, 1};
            
			FVector2d C = {p1.position.Z - a * p1.time * p1.time,
						 p2.position.Z - a * p2.time * p2.time};

			FVector2d B = A.Inverse() * C;

			double vx = (p2.position.X - p1.position.X) / (p2.time - p1.time);
			double vy = (p2.position.Y - p1.position.Y) / (p2.time - p1.time);

			return ParabPath(a, B.X, B.Y, p1.position.X - p1.time * vx, p1.position.Y - p1.time * vy, vx, vy, t0, t1);
		}

		static ParabPath from3Points(Position p1, Position p2, Position p3) {
			double t0 = p1.time;
			double t1 = p3.time;

			p1.time -= t0;
			p2.time -= t0;
			p3.time -= t0;
			
			TMatrix3<double> A;
			A.Row0 = {p1.time * p1.time, p1.time, 1};
			A.Row1 = {p2.time * p2.time, p2.time, 1};
			A.Row2 = {p3.time * p3.time, p3.time, 1};

			FVector C = {p1.position.Z, p2.position.Z, p3.position.Z};

			FVector B = A.Inverse() * C;

			double vx = (p3.position.X - p1.position.X) / (p3.time - p1.time);
			double vy = (p3.position.Y - p1.position.Y) / (p3.time - p1.time);
			
			return ParabPath(B.X, B.Y, B.Z, p1.position.X - p1.time * vx, p1.position.Y - p1.time * vy, vx, vy, t0, t1);
		}

		double derivative(double t) const {
			t -= t0;
			return 2 * a * t + b;
		}

		double derivative2() const {
			return 2 * a;
		}

		FVector operator()(double t) const {
			t -= t0;
			return {px + t * vx, py + t * vy, t * t * a + t * b + c};
		}
	};

	std::mutex ball_detection_2d_mut;
	void CameraLoop(ATrackingCamera* camera, std::deque<Detection>* ball_2d_detections);
	std::vector<TFuture<void>> camera_threads;

	// Thread handle. Control the thread using this, with operators like Kill and Suspend
	FRunnableThread* Thread;

	// Used to know when the thread should exit, changed in Stop(), read in Run()
	bool run_thread = true;
	bool thread_stopped = false;

	class ABall* ball;
};

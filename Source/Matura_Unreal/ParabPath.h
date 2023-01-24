// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <MatrixTypes.h>

#include <vector>

#include "CoreMinimal.h"

// doesn't compile without this stuff
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS
#include <Eigen/Eigen/Dense>
PRAGMA_ENABLE_SHADOW_VARIABLE_WARNINGS
#pragma clang diagnostic pop

using namespace std;


struct Position
{
	FVector position;
	double time;
};

class MATURA_UNREAL_API ParabPath
{
public:
	double a, b, c;
	double px, py;
	double vx, vy;
	double t0, t1;

	ParabPath(double a, double b, double c, double px, double py, double vx, double vy, double t0, double t1);

	ParabPath();

	static ParabPath from2Points(Position p1, Position p2, double a);

	static vector<double> LeastSquares(const Eigen::MatrixXd& data, int degree);

	static ParabPath fromNPoints(std::vector<Position> positions);
	
	vector<double> IntersectSphere(FVector center, double radius) const;

	double derivative(double t) const;

	double derivative2() const;

	void Draw(const UWorld* world, FColor color, double thickness, int depth_priority = 0, double lifetime = 1000);

	FVector operator()(double t) const;

	ParabPath& operator+=(double t);

	ParabPath operator+(double t) const;

	bool IsValid() const;
};

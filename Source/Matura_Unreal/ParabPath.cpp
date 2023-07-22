// Fill out your copyright notice in the Description page of Project Settings.


#include "ParabPath.h"

#include "quartic.h"

ParabPath::ParabPath(double a, double b, double c, double px, double py, double vx, double vy, double t0, double t1):
	a(a), b(b), c(c), px(px), py(py), vx(vx), vy(vy), t0(t0), t1(t1)
{
}

ParabPath::ParabPath()
{
	a = b = c = px = py = vx = vy = t0 = t1 = nan("");
}

ParabPath ParabPath::from2Points(Position p1, Position p2, double a)
{
	double t0 = p1.time;
	double t1 = p2.time;

	p1.time -= t0;
	p2.time -= t0;

	UE::Geometry::TMatrix2<double> A;
	A.Row0 = {p1.time, 1};
	A.Row1 = {p2.time, 1};

	FVector2d C = {
		p1.position.Z - a * p1.time * p1.time,
		p2.position.Z - a * p2.time * p2.time
	};

	FVector2d B = A.Inverse() * C;

	double vx = (p2.position.X - p1.position.X) / (p2.time - p1.time);
	double vy = (p2.position.Y - p1.position.Y) / (p2.time - p1.time);

	return ParabPath(a, B.X, B.Y, p1.position.X - p1.time * vx, p1.position.Y - p1.time * vy, vx, vy, t0, t1);
}

std::vector<double> ParabPath::LeastSquares(const Eigen::MatrixXd& data, int degree)
{
	assert(data.rows() >= degree);

	// https://www.reddit.com/r/cpp_questions/comments/v5oxql/polynomial_curve_fitting/
	Eigen::MatrixXd powers(data.rows(), degree + 1);
	powers.col(0).setOnes();

	for (int i = 1; i <= degree; i++)
		powers.col(i).array() = powers.col(i - 1).array() * data.col(0).array();

	auto decomposition = powers.householderQr();

	std::vector<double> coeffs(degree + 1);
	auto coeffs_map = Eigen::Map<Eigen::VectorXd>(coeffs.data(), coeffs.size());
	coeffs_map = decomposition.solve(data.col(1));

	return coeffs;
}

ParabPath ParabPath::fromNPoints(std::vector<Position> positions)
{
	assert(positions.size() >= 3);

	double t0 = positions[0].time;
	double t1 = positions.back().time;

	for (auto& [p, t] : positions)
		t -= t0;

	Eigen::MatrixXd dataX(positions.size(), 2);
	Eigen::MatrixXd dataY(positions.size(), 2);
	Eigen::MatrixXd dataZ(positions.size(), 2);

	for (int i = 0; i < positions.size(); i++)
	{
		Eigen::MatrixXd rowX, rowY, rowZ;
		rowX = rowY = rowZ = Eigen::MatrixXd(1, 2);

		rowX.data()[0] = rowY.data()[0] = rowZ.data()[0] = positions[i].time;
		rowX.data()[1] = positions[i].position.X;
		rowY.data()[1] = positions[i].position.Y;
		rowZ.data()[1] = positions[i].position.Z;

		dataX.row(i) = rowX, dataY.row(i) = rowY, dataZ.row(i) = rowZ;
	}

	// fit the data points to a parabola/line
	auto paramsX = LeastSquares(dataX, 1);
	auto paramsY = LeastSquares(dataY, 1);
	auto paramsZ = LeastSquares(dataZ, 2);

	return ParabPath(paramsZ[2], paramsZ[1], paramsZ[0], paramsX[0], paramsY[0], paramsX[1], paramsY[1], t0, t1);
}

std::vector<double> ParabPath::IntersectSphere(FVector center, double radius) const
{
	double A = a * a;
	double B = 2 * a * b;

	double C = vx * vx + vy * vy + 2 * a * c - 2 * a * center.Z + b * b;
	double D = 2 * (vx * px - vx * center.X + vy * py - vy * center.Y + b * c - b * center.Z);
	double E = px * px - 2 * px * center.X + center.X * center.X + py * py - 2 * py * center.Y + center.Y * center.Y +
		c * c - 2 * c * center.Z + center.Z * center.Z - radius * radius;

	std::vector<double> solutions = solve_quartic(A, B, C, D, E);

	for (auto& t : solutions)
		t += t0;

	return solutions;
}

double ParabPath::derivative(double t) const
{
	t -= t0;
	return 2 * a * t + b;
}

double ParabPath::derivative2() const
{
	return 2 * a;
}

void ParabPath::Draw(const UWorld* world, FColor color, double thickness, int depth_priority, double lifetime)
{
	for (double t = t0; t < t1; t += (t1 - t0) / 100)
	{
		DrawDebugLine(world, operator()(t), operator()(t + (t1 - t0) / 100), color, false, lifetime, depth_priority, thickness);
	}
}

FVector ParabPath::operator()(double t) const
{
	t -= t0;
	return {px + t * vx, py + t * vy, t * t * a + t * b + c};
}

ParabPath& ParabPath::operator+=(double t)
{
	t0 += t;
	t1 += t;
	px += t * vx;
	py += t * vy;

	c += a * t * t + b * t;
	b += 2 * a * t;

	return *this;
}

ParabPath ParabPath::operator+(double t) const
{
	ParabPath out = *this;
	out += t;
	return out;
}

bool ParabPath::IsValid() const
{
	return !isnan(a) && !isnan(b) && !isnan(c) && !isnan(vx) && !isnan(px) && !isnan(vy) && !isnan(py) && !isnan(t0) && !isnan(t1);
}

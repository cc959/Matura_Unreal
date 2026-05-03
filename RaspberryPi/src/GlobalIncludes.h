#pragma once

#define LogDisplay(x, ...) std::fprintf(stdout, "\033[34mDISPLAY:\033[0m " x "\n", ##__VA_ARGS__)
#define LogWarning(x, ...) std::fprintf(stdout, "\033[33mWARNING:\033[0m " x "\n", ##__VA_ARGS__)
#define LogErr(x, ...) std::fprintf(stderr, "\033[31mERROR:\033[0m " x "\n", ##__VA_ARGS__)
#define TEXT(x) x

#include <thread>
#include <Eigen/Dense>

typedef Eigen::Vector3d FVector;
typedef Eigen::Vector2d FVector2d;
typedef Eigen::Matrix3d FMatrix;
typedef Eigen::Quaterniond FQuaternion;
typedef Eigen::Affine3d FTransform;

#define NOW_NS std::chrono::high_resolution_clock::now().time_since_epoch().count()

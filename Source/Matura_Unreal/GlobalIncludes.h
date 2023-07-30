#pragma once

#define LogDisplay(x, ...) UE_LOG(LogTemp, Display, x, ##__VA_ARGS__)
#define LogWarning(x, ...) UE_LOG(LogTemp, Warning, x, ##__VA_ARGS__)
#define LogError(x, ...) UE_LOG(LogTemp, Error, x, ##__VA_ARGS__)

#include <thread>

#define usleep(x) std::this_thread::sleep_for(std::chrono::microseconds(uint64_t(x)));

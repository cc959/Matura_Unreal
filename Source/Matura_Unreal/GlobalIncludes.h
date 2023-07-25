#pragma once

#define LogDisplay(x, ...) UE_LOG(LogTemp, Display, x, ##__VA_ARGS__)
#define LogWarning(x, ...) UE_LOG(LogTemp, Warning, x, ##__VA_ARGS__)
#define LogError(x, ...) UE_LOG(LogTemp, Error, x, ##__VA_ARGS__)

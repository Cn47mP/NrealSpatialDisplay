#pragma once
#include <cstdint>
#include <chrono>

inline uint64_t GetTimeUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

inline uint64_t GetTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

inline double GetTimeSec() {
    return GetTimeUs() / 1e6;
}
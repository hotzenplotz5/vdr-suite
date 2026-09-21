#pragma once

#include <cstddef>
#include <string>

struct RuntimeMeasurement {
    std::string component;
    std::string operation;
    long long durationMs = 0;
    int statusCode = 0;
    std::size_t sizeBytes = 0;
    std::size_t itemCount = 0;
};

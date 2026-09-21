#pragma once

#include <cstddef>
#include <string>

struct RuntimeMeasurementSummary {
    std::string component;
    std::string operation;
    std::size_t count = 0;
    long long minDurationMs = 0;
    long long maxDurationMs = 0;
    long long lastDurationMs = 0;
    int lastStatusCode = 0;
    std::size_t lastSizeBytes = 0;
    std::size_t lastItemCount = 0;
};

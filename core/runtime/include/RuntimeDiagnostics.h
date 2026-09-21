#pragma once

#include "RuntimeMeasurement.h"

#include <vector>

struct RuntimeDiagnostics {
    std::vector<RuntimeMeasurement> measurements;

    void addMeasurement(const RuntimeMeasurement& measurement)
    {
        measurements.push_back(measurement);
    }

    bool empty() const
    {
        return measurements.empty();
    }

    std::size_t size() const
    {
        return measurements.size();
    }

    void clear()
    {
        measurements.clear();
    }
};

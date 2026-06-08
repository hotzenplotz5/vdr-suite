#pragma once

#include "RuntimeMeasurement.h"

class IRuntimeMeasurementSink {
public:
    virtual ~IRuntimeMeasurementSink() = default;

    virtual void recordMeasurement(const RuntimeMeasurement& measurement) = 0;
};

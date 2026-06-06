#pragma once

#include "IRuntimeMeasurementSink.h"
#include "RuntimeDiagnostics.h"
#include "RuntimeMeasurement.h"

#include <cstddef>

class RuntimeDiagnosticsService : public IRuntimeMeasurementSink {
public:
    explicit RuntimeDiagnosticsService(std::size_t maxMeasurements = 100)
        : maxMeasurements_(maxMeasurements)
    {
    }

    void recordMeasurement(const RuntimeMeasurement& measurement) override
    {
        diagnostics_.addMeasurement(measurement);

        while (diagnostics_.size() > maxMeasurements_) {
            diagnostics_.measurements.erase(diagnostics_.measurements.begin());
        }
    }

    const RuntimeDiagnostics& diagnostics() const
    {
        return diagnostics_;
    }

    bool empty() const
    {
        return diagnostics_.empty();
    }

    std::size_t size() const
    {
        return diagnostics_.size();
    }

    void clear()
    {
        diagnostics_.clear();
    }

private:
    std::size_t maxMeasurements_;
    RuntimeDiagnostics diagnostics_;
};

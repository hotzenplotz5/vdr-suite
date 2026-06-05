#pragma once

#include "RuntimeDiagnostics.h"
#include "RuntimeMeasurement.h"

class RuntimeDiagnosticsService {
public:
    void recordMeasurement(const RuntimeMeasurement& measurement)
    {
        diagnostics_.addMeasurement(measurement);
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
    RuntimeDiagnostics diagnostics_;
};

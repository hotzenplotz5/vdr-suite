#pragma once

#include "IRuntimeMeasurementSink.h"
#include "RuntimeDiagnostics.h"
#include "RuntimeMeasurement.h"
#include "RuntimeMeasurementSummary.h"

#include <cstddef>
#include <vector>

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

    std::vector<RuntimeMeasurementSummary> measurementSummaries() const
    {
        std::vector<RuntimeMeasurementSummary> summaries;

        for (const RuntimeMeasurement& measurement : diagnostics_.measurements) {
            RuntimeMeasurementSummary* summary = nullptr;

            for (RuntimeMeasurementSummary& existingSummary : summaries) {
                if (existingSummary.component == measurement.component
                    && existingSummary.operation == measurement.operation) {
                    summary = &existingSummary;
                    break;
                }
            }

            if (summary == nullptr) {
                RuntimeMeasurementSummary newSummary;
                newSummary.component = measurement.component;
                newSummary.operation = measurement.operation;
                summaries.push_back(newSummary);
                summary = &summaries.back();
            }

            if (summary->count == 0) {
                summary->minDurationMs = measurement.durationMs;
                summary->maxDurationMs = measurement.durationMs;
            }

            if (measurement.durationMs < summary->minDurationMs) {
                summary->minDurationMs = measurement.durationMs;
            }

            if (measurement.durationMs > summary->maxDurationMs) {
                summary->maxDurationMs = measurement.durationMs;
            }

            summary->count += 1;
            summary->lastDurationMs = measurement.durationMs;
            summary->lastStatusCode = measurement.statusCode;
            summary->lastSizeBytes = measurement.sizeBytes;
            summary->lastItemCount = measurement.itemCount;
        }

        return summaries;
    }

private:
    std::size_t maxMeasurements_;
    RuntimeDiagnostics diagnostics_;
};

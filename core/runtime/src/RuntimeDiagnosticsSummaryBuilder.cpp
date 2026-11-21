#include "RuntimeDiagnosticsSummaryBuilder.h"

#include <map>
#include <string>
#include <utility>

std::vector<RuntimeMeasurementSummary> RuntimeDiagnosticsSummaryBuilder::build(
    const RuntimeDiagnostics& diagnostics) const
{
    std::map<std::pair<std::string, std::string>, RuntimeMeasurementSummary> summariesByKey;

    for (const auto& measurement : diagnostics.measurements) {
        const auto key =
            std::make_pair(measurement.component, measurement.operation);

        auto iterator = summariesByKey.find(key);

        if (iterator == summariesByKey.end()) {
            RuntimeMeasurementSummary summary;
            summary.component = measurement.component;
            summary.operation = measurement.operation;
            summary.count = 1;
            summary.minDurationMs = measurement.durationMs;
            summary.maxDurationMs = measurement.durationMs;
            summary.lastDurationMs = measurement.durationMs;
            summary.lastStatusCode = measurement.statusCode;
            summary.lastSizeBytes = measurement.sizeBytes;
            summary.lastItemCount = measurement.itemCount;

            summariesByKey.emplace(key, summary);
            continue;
        }

        RuntimeMeasurementSummary& summary = iterator->second;
        ++summary.count;

        if (measurement.durationMs < summary.minDurationMs) {
            summary.minDurationMs = measurement.durationMs;
        }

        if (measurement.durationMs > summary.maxDurationMs) {
            summary.maxDurationMs = measurement.durationMs;
        }

        summary.lastDurationMs = measurement.durationMs;
        summary.lastStatusCode = measurement.statusCode;
        summary.lastSizeBytes = measurement.sizeBytes;
        summary.lastItemCount = measurement.itemCount;
    }

    std::vector<RuntimeMeasurementSummary> summaries;

    for (const auto& entry : summariesByKey) {
        summaries.push_back(entry.second);
    }

    return summaries;
}

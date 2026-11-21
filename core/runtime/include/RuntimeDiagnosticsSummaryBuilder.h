#pragma once

#include "RuntimeDiagnostics.h"
#include "RuntimeMeasurementSummary.h"

#include <vector>

class RuntimeDiagnosticsSummaryBuilder {
public:
    std::vector<RuntimeMeasurementSummary> build(
        const RuntimeDiagnostics& diagnostics) const;
};

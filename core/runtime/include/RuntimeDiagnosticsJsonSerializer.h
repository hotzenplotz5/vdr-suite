#pragma once

#include "RuntimeDiagnostics.h"
#include "RuntimeMeasurementSummary.h"

#include <string>
#include <vector>

class RuntimeDiagnosticsJsonSerializer {
public:
    std::string serialize(const RuntimeDiagnostics& diagnostics) const;
    std::string serialize(const std::vector<RuntimeMeasurementSummary>& summaries) const;
};

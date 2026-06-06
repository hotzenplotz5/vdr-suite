#pragma once

#include "RuntimeDiagnostics.h"

#include <string>

class RuntimeDiagnosticsJsonSerializer {
public:
    std::string serialize(const RuntimeDiagnostics& diagnostics) const;
};

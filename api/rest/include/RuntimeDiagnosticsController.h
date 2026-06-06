#pragma once

#include "DashboardController.h"

class RuntimeDiagnosticsJsonSerializer;
class RuntimeDiagnosticsService;

class RuntimeDiagnosticsController
{
public:
    RuntimeDiagnosticsController(
        RuntimeDiagnosticsService& diagnosticsService,
        RuntimeDiagnosticsJsonSerializer& jsonSerializer);

    ApiResponse getRuntimeDiagnostics();

private:
    RuntimeDiagnosticsService& diagnosticsService_;
    RuntimeDiagnosticsJsonSerializer& jsonSerializer_;
};

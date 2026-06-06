#include "RuntimeDiagnosticsController.h"

#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"

RuntimeDiagnosticsController::RuntimeDiagnosticsController(
    RuntimeDiagnosticsService& diagnosticsService,
    RuntimeDiagnosticsJsonSerializer& jsonSerializer)
    : diagnosticsService_(diagnosticsService),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse RuntimeDiagnosticsController::getRuntimeDiagnostics()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(
            diagnosticsService_.diagnostics());

    return response;
}

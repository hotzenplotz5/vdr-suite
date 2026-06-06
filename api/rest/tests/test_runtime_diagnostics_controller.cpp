#include "RuntimeDiagnosticsController.h"

#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "RuntimeMeasurement.h"

#include <cassert>
#include <iostream>
#include <string>

static void test_runtime_diagnostics_controller_returns_empty_diagnostics()
{
    RuntimeDiagnosticsService diagnosticsService;
    RuntimeDiagnosticsJsonSerializer jsonSerializer;

    RuntimeDiagnosticsController controller(
        diagnosticsService,
        jsonSerializer);

    ApiResponse response =
        controller.getRuntimeDiagnostics();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body == "{\"measurements\":[]}");
}

static void test_runtime_diagnostics_controller_returns_recorded_measurements()
{
    RuntimeDiagnosticsService diagnosticsService;
    RuntimeDiagnosticsJsonSerializer jsonSerializer;

    RuntimeMeasurement measurement;
    measurement.component = "PollingService";
    measurement.operation = "Poll cycle";
    measurement.durationMs = 42;
    measurement.statusCode = 0;
    measurement.sizeBytes = 0;

    diagnosticsService.recordMeasurement(measurement);

    RuntimeDiagnosticsController controller(
        diagnosticsService,
        jsonSerializer);

    ApiResponse response =
        controller.getRuntimeDiagnostics();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");

    assert(response.body.find("\"measurements\"")
           != std::string::npos);

    assert(response.body.find("\"component\":\"PollingService\"")
           != std::string::npos);

    assert(response.body.find("\"operation\":\"Poll cycle\"")
           != std::string::npos);

    assert(response.body.find("\"durationMs\":42")
           != std::string::npos);
}

int main()
{
    test_runtime_diagnostics_controller_returns_empty_diagnostics();
    test_runtime_diagnostics_controller_returns_recorded_measurements();

    std::cout
        << "test_runtime_diagnostics_controller passed"
        << std::endl;

    return 0;
}

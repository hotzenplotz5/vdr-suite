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

static void test_runtime_diagnostics_controller_returns_empty_summary()
{
    RuntimeDiagnosticsService diagnosticsService;
    RuntimeDiagnosticsJsonSerializer jsonSerializer;

    RuntimeDiagnosticsController controller(
        diagnosticsService,
        jsonSerializer);

    ApiResponse response =
        controller.getRuntimeDiagnosticsSummary();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body == "{\"summaries\":[]}");
}

static void test_runtime_diagnostics_controller_returns_recorded_summary()
{
    RuntimeDiagnosticsService diagnosticsService;
    RuntimeDiagnosticsJsonSerializer jsonSerializer;

    RuntimeMeasurement firstMeasurement;
    firstMeasurement.component = "PollingService";
    firstMeasurement.operation = "Poll cycle";
    firstMeasurement.durationMs = 30;
    firstMeasurement.statusCode = 0;
    firstMeasurement.sizeBytes = 100;

    RuntimeMeasurement secondMeasurement;
    secondMeasurement.component = "PollingService";
    secondMeasurement.operation = "Poll cycle";
    secondMeasurement.durationMs = 10;
    secondMeasurement.statusCode = 0;
    secondMeasurement.sizeBytes = 200;

    diagnosticsService.recordMeasurement(firstMeasurement);
    diagnosticsService.recordMeasurement(secondMeasurement);

    RuntimeDiagnosticsController controller(
        diagnosticsService,
        jsonSerializer);

    ApiResponse response =
        controller.getRuntimeDiagnosticsSummary();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");

    assert(response.body.find("\"summaries\"")
           != std::string::npos);

    assert(response.body.find("\"component\":\"PollingService\"")
           != std::string::npos);

    assert(response.body.find("\"operation\":\"Poll cycle\"")
           != std::string::npos);

    assert(response.body.find("\"count\":2")
           != std::string::npos);

    assert(response.body.find("\"minDurationMs\":10")
           != std::string::npos);

    assert(response.body.find("\"maxDurationMs\":30")
           != std::string::npos);
}

int main()
{
    test_runtime_diagnostics_controller_returns_empty_diagnostics();
    test_runtime_diagnostics_controller_returns_recorded_measurements();
    test_runtime_diagnostics_controller_returns_empty_summary();
    test_runtime_diagnostics_controller_returns_recorded_summary();

    std::cout
        << "test_runtime_diagnostics_controller passed"
        << std::endl;

    return 0;
}

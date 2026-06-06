#include "RuntimeDiagnostics.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeMeasurement.h"

#include <cassert>
#include <iostream>
#include <string>

static void test_empty_diagnostics_serializes_empty_measurement_array()
{
    RuntimeDiagnostics diagnostics;
    RuntimeDiagnosticsJsonSerializer serializer;

    const std::string json = serializer.serialize(diagnostics);

    assert(json == "{\"measurements\":[]}");
}

static void test_diagnostics_serializes_measurements()
{
    RuntimeDiagnostics diagnostics;

    RuntimeMeasurement first;
    first.component = "PollingService";
    first.operation = "Poll cycle";
    first.durationMs = 12;
    first.statusCode = 0;
    first.sizeBytes = 0;
    diagnostics.addMeasurement(first);

    RuntimeMeasurement second;
    second.component = "BasicHttpClient";
    second.operation = "GET /recordings.json";
    second.durationMs = 345;
    second.statusCode = 200;
    second.sizeBytes = 12345;
    diagnostics.addMeasurement(second);

    RuntimeDiagnosticsJsonSerializer serializer;
    const std::string json = serializer.serialize(diagnostics);

    assert(json.find("\"measurements\"") != std::string::npos);
    assert(json.find("\"component\":\"PollingService\"") != std::string::npos);
    assert(json.find("\"operation\":\"Poll cycle\"") != std::string::npos);
    assert(json.find("\"durationMs\":12") != std::string::npos);
    assert(json.find("\"statusCode\":200") != std::string::npos);
    assert(json.find("\"sizeBytes\":12345") != std::string::npos);
}

static void test_diagnostics_serializer_escapes_strings()
{
    RuntimeDiagnostics diagnostics;

    RuntimeMeasurement measurement;
    measurement.component = "Component\"Name";
    measurement.operation = "GET \\ path\nnext";
    measurement.durationMs = 1;
    diagnostics.addMeasurement(measurement);

    RuntimeDiagnosticsJsonSerializer serializer;
    const std::string json = serializer.serialize(diagnostics);

    assert(json.find("Component\\\"Name") != std::string::npos);
    assert(json.find("GET \\\\ path\\nnext") != std::string::npos);
}

int main()
{
    test_empty_diagnostics_serializes_empty_measurement_array();
    test_diagnostics_serializes_measurements();
    test_diagnostics_serializer_escapes_strings();

    std::cout << "test_runtime_diagnostics_json_serializer passed" << std::endl;
    return 0;
}

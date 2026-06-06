#include "RuntimeDiagnostics.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeMeasurement.h"
#include "RuntimeMeasurementSummary.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

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
    first.itemCount = 3;
    diagnostics.addMeasurement(first);

    RuntimeMeasurement second;
    second.component = "BasicHttpClient";
    second.operation = "GET /recordings.json";
    second.durationMs = 345;
    second.statusCode = 200;
    second.sizeBytes = 12345;
    second.itemCount = 27;
    diagnostics.addMeasurement(second);

    RuntimeDiagnosticsJsonSerializer serializer;
    const std::string json = serializer.serialize(diagnostics);

    assert(json.find("\"measurements\"") != std::string::npos);
    assert(json.find("\"component\":\"PollingService\"") != std::string::npos);
    assert(json.find("\"operation\":\"Poll cycle\"") != std::string::npos);
    assert(json.find("\"durationMs\":12") != std::string::npos);
    assert(json.find("\"statusCode\":200") != std::string::npos);
    assert(json.find("\"sizeBytes\":12345") != std::string::npos);
    assert(json.find("\"itemCount\":27") != std::string::npos);
}

static void test_summary_serializer_serializes_empty_summary_array()
{
    std::vector<RuntimeMeasurementSummary> summaries;
    RuntimeDiagnosticsJsonSerializer serializer;

    const std::string json = serializer.serialize(summaries);

    assert(json == "{\"summaries\":[]}");
}

static void test_summary_serializer_serializes_measurement_summaries()
{
    RuntimeMeasurementSummary summary;
    summary.component = "PollingService";
    summary.operation = "Poll cycle";
    summary.count = 2;
    summary.minDurationMs = 10;
    summary.maxDurationMs = 30;
    summary.lastDurationMs = 10;
    summary.lastStatusCode = 0;
    summary.lastSizeBytes = 200;
    summary.lastItemCount = 9;

    std::vector<RuntimeMeasurementSummary> summaries;
    summaries.push_back(summary);

    RuntimeDiagnosticsJsonSerializer serializer;
    const std::string json = serializer.serialize(summaries);

    assert(json.find("\"summaries\"") != std::string::npos);
    assert(json.find("\"component\":\"PollingService\"") != std::string::npos);
    assert(json.find("\"operation\":\"Poll cycle\"") != std::string::npos);
    assert(json.find("\"count\":2") != std::string::npos);
    assert(json.find("\"minDurationMs\":10") != std::string::npos);
    assert(json.find("\"maxDurationMs\":30") != std::string::npos);
    assert(json.find("\"lastDurationMs\":10") != std::string::npos);
    assert(json.find("\"lastStatusCode\":0") != std::string::npos);
    assert(json.find("\"lastSizeBytes\":200") != std::string::npos);
    assert(json.find("\"lastItemCount\":9") != std::string::npos);
}

int main()
{
    test_empty_diagnostics_serializes_empty_measurement_array();
    test_diagnostics_serializes_measurements();
    test_summary_serializer_serializes_empty_summary_array();
    test_summary_serializer_serializes_measurement_summaries();

    std::cout << "test_runtime_diagnostics_json_serializer passed" << std::endl;
    return 0;
}

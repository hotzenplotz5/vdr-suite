#include "RuntimeDiagnosticsService.h"

#include <cassert>

int main()
{
    RuntimeDiagnosticsService service;

    assert(service.empty());
    assert(service.size() == 0);
    assert(service.diagnostics().empty());

    RuntimeMeasurement httpMeasurement;
    httpMeasurement.component = "BasicHttpClient";
    httpMeasurement.operation = "GET /recordings.json";
    httpMeasurement.durationMs = 10634;
    httpMeasurement.statusCode = 200;
    httpMeasurement.sizeBytes = 4343857;

    service.recordMeasurement(httpMeasurement);

    assert(!service.empty());
    assert(service.size() == 1);
    assert(!service.diagnostics().empty());
    assert(service.diagnostics().measurements.at(0).component == "BasicHttpClient");
    assert(service.diagnostics().measurements.at(0).operation == "GET /recordings.json");
    assert(service.diagnostics().measurements.at(0).durationMs == 10634);
    assert(service.diagnostics().measurements.at(0).statusCode == 200);
    assert(service.diagnostics().measurements.at(0).sizeBytes == 4343857);

    RuntimeMeasurement snapshotMeasurement;
    snapshotMeasurement.component = "VdrSnapshotBuilder";
    snapshotMeasurement.operation = "Build recordings";
    snapshotMeasurement.durationMs = 11047;

    service.recordMeasurement(snapshotMeasurement);

    assert(service.size() == 2);
    assert(service.diagnostics().measurements.at(1).component == "VdrSnapshotBuilder");
    assert(service.diagnostics().measurements.at(1).operation == "Build recordings");
    assert(service.diagnostics().measurements.at(1).durationMs == 11047);

    service.clear();

    assert(service.empty());
    assert(service.size() == 0);
    assert(service.diagnostics().empty());

    return 0;
}

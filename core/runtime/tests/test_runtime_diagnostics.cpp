#include "RuntimeDiagnostics.h"

#include <cassert>

int main()
{
    RuntimeDiagnostics diagnostics;

    assert(diagnostics.empty());
    assert(diagnostics.size() == 0);

    RuntimeMeasurement measurement;
    measurement.component = "BasicHttpClient";
    measurement.operation = "GET /channels.json";
    measurement.durationMs = 12;
    measurement.statusCode = 200;
    measurement.sizeBytes = 4096;

    diagnostics.addMeasurement(measurement);

    assert(!diagnostics.empty());
    assert(diagnostics.size() == 1);
    assert(diagnostics.measurements.at(0).component == "BasicHttpClient");
    assert(diagnostics.measurements.at(0).operation == "GET /channels.json");
    assert(diagnostics.measurements.at(0).durationMs == 12);
    assert(diagnostics.measurements.at(0).statusCode == 200);
    assert(diagnostics.measurements.at(0).sizeBytes == 4096);

    diagnostics.clear();

    assert(diagnostics.empty());
    assert(diagnostics.size() == 0);

    return 0;
}

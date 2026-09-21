#include "RuntimeDiagnosticsService.h"

#include <cassert>
#include <vector>

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
    assert(service.diagnostics().measurements.at(0).itemCount == 0);

    RuntimeMeasurement snapshotMeasurement;
    snapshotMeasurement.component = "VdrSnapshotBuilder";
    snapshotMeasurement.operation = "Build recordings";
    snapshotMeasurement.durationMs = 11047;

    service.recordMeasurement(snapshotMeasurement);

    assert(service.size() == 2);
    assert(service.diagnostics().measurements.at(1).component == "VdrSnapshotBuilder");
    assert(service.diagnostics().measurements.at(1).operation == "Build recordings");
    assert(service.diagnostics().measurements.at(1).durationMs == 11047);

    RuntimeDiagnosticsService limitedService(2);

    RuntimeMeasurement firstMeasurement;
    firstMeasurement.component = "FirstComponent";
    firstMeasurement.operation = "First operation";

    RuntimeMeasurement secondMeasurement;
    secondMeasurement.component = "SecondComponent";
    secondMeasurement.operation = "Second operation";

    RuntimeMeasurement thirdMeasurement;
    thirdMeasurement.component = "ThirdComponent";
    thirdMeasurement.operation = "Third operation";

    limitedService.recordMeasurement(firstMeasurement);
    limitedService.recordMeasurement(secondMeasurement);
    limitedService.recordMeasurement(thirdMeasurement);

    assert(limitedService.size() == 2);
    assert(limitedService.diagnostics().measurements.at(0).component == "SecondComponent");
    assert(limitedService.diagnostics().measurements.at(0).operation == "Second operation");
    assert(limitedService.diagnostics().measurements.at(1).component == "ThirdComponent");
    assert(limitedService.diagnostics().measurements.at(1).operation == "Third operation");

    {
        RuntimeDiagnosticsService aggregationService;

        RuntimeMeasurement firstPollMeasurement;
        firstPollMeasurement.component = "PollingService";
        firstPollMeasurement.operation = "Poll cycle";
        firstPollMeasurement.durationMs = 30;
        firstPollMeasurement.statusCode = 0;
        firstPollMeasurement.itemCount = 100;

        RuntimeMeasurement secondPollMeasurement;
        secondPollMeasurement.component = "PollingService";
        secondPollMeasurement.operation = "Poll cycle";
        secondPollMeasurement.durationMs = 10;
        secondPollMeasurement.statusCode = 0;
        secondPollMeasurement.itemCount = 200;

        RuntimeMeasurement httpMeasurement;
        httpMeasurement.component = "BasicHttpClient";
        httpMeasurement.operation = "GET /recordings.json";
        httpMeasurement.durationMs = 50;
        httpMeasurement.statusCode = 200;
        httpMeasurement.sizeBytes = 300;

        aggregationService.recordMeasurement(firstPollMeasurement);
        aggregationService.recordMeasurement(secondPollMeasurement);
        aggregationService.recordMeasurement(httpMeasurement);

        const std::vector<RuntimeMeasurementSummary> summaries =
            aggregationService.measurementSummaries();

        assert(summaries.size() == 2);

        assert(summaries.at(0).component == "PollingService");
        assert(summaries.at(0).operation == "Poll cycle");
        assert(summaries.at(0).count == 2);
        assert(summaries.at(0).minDurationMs == 10);
        assert(summaries.at(0).maxDurationMs == 30);
        assert(summaries.at(0).lastDurationMs == 10);
        assert(summaries.at(0).lastStatusCode == 0);
        assert(summaries.at(0).lastSizeBytes == 0);
        assert(summaries.at(0).lastItemCount == 200);

        assert(summaries.at(1).component == "BasicHttpClient");
        assert(summaries.at(1).operation == "GET /recordings.json");
        assert(summaries.at(1).count == 1);
        assert(summaries.at(1).minDurationMs == 50);
        assert(summaries.at(1).maxDurationMs == 50);
        assert(summaries.at(1).lastDurationMs == 50);
        assert(summaries.at(1).lastStatusCode == 200);
        assert(summaries.at(1).lastSizeBytes == 300);
        assert(summaries.at(1).lastItemCount == 0);
    }

    service.clear();

    assert(service.empty());
    assert(service.size() == 0);
    assert(service.diagnostics().empty());

    return 0;
}

#include "RuntimeDiagnostics.h"
#include "RuntimeDiagnosticsSummaryBuilder.h"

#include <cassert>
#include <iostream>
#include <vector>

static void test_empty_diagnostics_creates_empty_summary()
{
    RuntimeDiagnostics diagnostics;
    RuntimeDiagnosticsSummaryBuilder builder;

    const auto summaries = builder.build(diagnostics);

    assert(summaries.empty());
}

static void test_builder_groups_measurements_by_component_and_operation()
{
    RuntimeDiagnostics diagnostics;

    RuntimeMeasurement first;
    first.component = "PollingService";
    first.operation = "Selective events merge refresh";
    first.durationMs = 12;
    first.itemCount = 3;
    diagnostics.addMeasurement(first);

    RuntimeMeasurement second;
    second.component = "PollingService";
    second.operation = "Selective events merge refresh";
    second.durationMs = 30;
    second.itemCount = 7;
    diagnostics.addMeasurement(second);

    RuntimeMeasurement third;
    third.component = "PollingService";
    third.operation = "Full events refresh";
    third.durationMs = 90;
    third.itemCount = 1000;
    diagnostics.addMeasurement(third);

    RuntimeDiagnosticsSummaryBuilder builder;
    const auto summaries = builder.build(diagnostics);

    assert(summaries.size() == 2);

    const RuntimeMeasurementSummary& fullRefreshSummary = summaries[0];
    const RuntimeMeasurementSummary& selectiveSummary = summaries[1];

    assert(fullRefreshSummary.component == "PollingService");
    assert(fullRefreshSummary.operation == "Full events refresh");
    assert(fullRefreshSummary.count == 1);
    assert(fullRefreshSummary.minDurationMs == 90);
    assert(fullRefreshSummary.maxDurationMs == 90);
    assert(fullRefreshSummary.lastDurationMs == 90);
    assert(fullRefreshSummary.lastItemCount == 1000);

    assert(selectiveSummary.component == "PollingService");
    assert(selectiveSummary.operation == "Selective events merge refresh");
    assert(selectiveSummary.count == 2);
    assert(selectiveSummary.minDurationMs == 12);
    assert(selectiveSummary.maxDurationMs == 30);
    assert(selectiveSummary.lastDurationMs == 30);
    assert(selectiveSummary.lastItemCount == 7);
}

int main()
{
    test_empty_diagnostics_creates_empty_summary();
    test_builder_groups_measurements_by_component_and_operation();

    std::cout << "test_runtime_diagnostics_summary_builder passed" << std::endl;
    return 0;
}

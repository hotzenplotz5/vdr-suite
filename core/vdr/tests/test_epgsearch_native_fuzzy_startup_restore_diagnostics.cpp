#include "EpgSearchNativeFuzzyStartupRestoreDiagnostics.h"
#include "EpgSearchNativeFuzzyStartupRestoreDiagnosticsJsonSerializer.h"

#include <cassert>
#include <iostream>

static void test_schema_unavailable_diagnostic()
{
    EpgSearchNativeFuzzyStartupRestoreSummary summary;
    summary.schemaReady = false;

    const auto diagnostics =
        EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(summary);
    const auto measurement = diagnostics.toRuntimeMeasurement();

    assert(!diagnostics.schemaReady);
    assert(diagnostics.status() == "schema-unavailable");
    assert(diagnostics.reason() == "persistence schema was not available");
    assert(measurement.component == "epgsearch-native-fuzzy");
    assert(measurement.operation == "startup-restore");
    assert(measurement.statusCode == 503);
    assert(measurement.itemCount == 0);
}

static void test_no_persisted_results_diagnostic()
{
    EpgSearchNativeFuzzyStartupRestoreSummary summary;
    summary.schemaReady = true;
    summary.backendsSeen = 1;
    summary.persistedResultsFound = 0;

    const auto diagnostics =
        EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(summary);
    const auto measurement = diagnostics.toRuntimeMeasurement();

    assert(diagnostics.status() == "no-persisted-results");
    assert(measurement.statusCode == 204);
    assert(measurement.itemCount == 0);
}

static void test_restored_native_available_diagnostic()
{
    EpgSearchNativeFuzzyStartupRestoreSummary summary;
    summary.schemaReady = true;
    summary.backendsSeen = 2;
    summary.persistedResultsFound = 2;
    summary.backendsUpdated = 2;
    summary.nativeFuzzyAvailable = 1;

    const auto diagnostics =
        EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(summary);
    const auto measurement = diagnostics.toRuntimeMeasurement();

    assert(diagnostics.status() == "restored-native-available");
    assert(diagnostics.nativeFuzzyUnavailable == 1);
    assert(measurement.statusCode == 200);
    assert(measurement.itemCount == 2);
}

static void test_restored_native_unavailable_diagnostic()
{
    EpgSearchNativeFuzzyStartupRestoreSummary summary;
    summary.schemaReady = true;
    summary.backendsSeen = 2;
    summary.persistedResultsFound = 2;
    summary.backendsUpdated = 2;
    summary.nativeFuzzyAvailable = 0;

    const auto diagnostics =
        EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(summary);

    assert(diagnostics.status() == "restored-native-unavailable");
    assert(diagnostics.nativeFuzzyUnavailable == 2);
}

static void test_stale_results_ignored_diagnostic()
{
    EpgSearchNativeFuzzyStartupRestoreSummary summary;
    summary.schemaReady = true;
    summary.backendsSeen = 1;
    summary.persistedResultsFound = 1;
    summary.backendsUpdated = 1;
    summary.nativeFuzzyAvailable = 0;
    summary.staleResultsIgnored = 1;

    const auto diagnostics =
        EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(summary);

    assert(diagnostics.status() == "stale-results-ignored");
    assert(diagnostics.staleResultsIgnored == 1);
    assert(diagnostics.nativeFuzzyUnavailable == 1);
}

static void test_json_serializer()
{
    EpgSearchNativeFuzzyStartupRestoreSummary summary;
    summary.schemaReady = true;
    summary.backendsSeen = 2;
    summary.persistedResultsFound = 2;
    summary.backendsUpdated = 2;
    summary.nativeFuzzyAvailable = 1;

    const auto diagnostics =
        EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(summary);

    EpgSearchNativeFuzzyStartupRestoreDiagnosticsJsonSerializer serializer;
    const auto json = serializer.serialize(diagnostics);

    assert(json.find("\"schemaReady\":true") != std::string::npos);
    assert(json.find("\"backendsSeen\":2") != std::string::npos);
    assert(json.find("\"persistedResultsFound\":2") != std::string::npos);
    assert(json.find("\"nativeFuzzyAvailable\":1") != std::string::npos);
    assert(json.find("\"nativeFuzzyUnavailable\":1") != std::string::npos);
    assert(json.find("\"staleResultsIgnored\":0") != std::string::npos);
    assert(json.find("\"status\":\"restored-native-available\"") != std::string::npos);
}

int main()
{
    test_schema_unavailable_diagnostic();
    test_no_persisted_results_diagnostic();
    test_restored_native_available_diagnostic();
    test_restored_native_unavailable_diagnostic();
    test_stale_results_ignored_diagnostic();
    test_json_serializer();

    std::cout
        << "test_epgsearch_native_fuzzy_startup_restore_diagnostics passed"
        << std::endl;

    return 0;
}

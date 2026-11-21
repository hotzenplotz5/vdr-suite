#include "Database.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityFreshnessPolicy.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyStaleProbeAdministrationService.h"

#include <cassert>
#include <cstdio>
#include <iostream>

static const char* dbPath = "/tmp/vdr-suite-native-fuzzy-stale-probe-admin-test.db";

static Database openDatabase()
{
    std::remove(dbPath);

    Database database;
    assert(database.open(dbPath));
    assert(database.isOpen());

    return database;
}

static void test_list_stale_probe_results()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy freshnessPolicy(60);
    EpgSearchNativeFuzzyStaleProbeAdministrationService service(
        repository,
        freshnessPolicy);

    assert(repository.ensureSchema());

    assert(repository.save(
        "fresh",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));
    assert(repository.save(
        "stale",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));
    assert(repository.save(
        "future",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));

    assert(database.execute(
        "UPDATE epgsearch_native_fuzzy_capability_probes "
        "SET updated_at = datetime('now', '-2 minutes') "
        "WHERE backend_id = 'stale';"));

    assert(database.execute(
        "UPDATE epgsearch_native_fuzzy_capability_probes "
        "SET updated_at = datetime('now', '+1 minute') "
        "WHERE backend_id = 'future';"));

    const auto staleResults = service.listStaleProbeResults();

    assert(staleResults.size() == 2);

    bool foundStale = false;
    bool foundFuture = false;

    for (const auto& record : staleResults)
    {
        if (record.backendId == "stale")
        {
            foundStale = true;
            assert(record.status == "stale");
            assert(record.ageSeconds > record.maxAgeSeconds);
        }

        if (record.backendId == "future")
        {
            foundFuture = true;
            assert(record.status == "future-timestamp");
            assert(record.ageSeconds < 0);
        }
    }

    assert(foundStale);
    assert(foundFuture);
}

static void test_delete_stale_probe_results_keeps_fresh_results()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy freshnessPolicy(60);
    EpgSearchNativeFuzzyStaleProbeAdministrationService service(
        repository,
        freshnessPolicy);

    assert(repository.ensureSchema());

    assert(repository.save(
        "fresh",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));
    assert(repository.save(
        "stale",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));

    assert(database.execute(
        "UPDATE epgsearch_native_fuzzy_capability_probes "
        "SET updated_at = datetime('now', '-2 minutes') "
        "WHERE backend_id = 'stale';"));

    const auto summary = service.deleteStaleProbeResults();

    assert(summary.schemaReady);
    assert(summary.persistedResultsSeen == 2);
    assert(summary.staleResultsFound == 1);
    assert(summary.deletedResults == 1);
    assert(summary.deleteFailures == 0);

    assert(repository.loadPersistedProbeResult("fresh").has_value());
    assert(!repository.loadPersistedProbeResult("stale").has_value());

    const auto staleResultsAfterDelete = service.listStaleProbeResults();
    assert(staleResultsAfterDelete.empty());
}

static void test_delete_stale_probe_results_creates_schema_safely()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy freshnessPolicy(60);
    EpgSearchNativeFuzzyStaleProbeAdministrationService service(
        repository,
        freshnessPolicy);

    assert(!database.tableExists("epgsearch_native_fuzzy_capability_probes"));

    const auto summary = service.deleteStaleProbeResults();

    assert(summary.schemaReady);
    assert(summary.persistedResultsSeen == 0);
    assert(summary.staleResultsFound == 0);
    assert(summary.deletedResults == 0);
    assert(summary.deleteFailures == 0);
    assert(database.tableExists("epgsearch_native_fuzzy_capability_probes"));
}

int main()
{
    test_list_stale_probe_results();
    test_delete_stale_probe_results_keeps_fresh_results();
    test_delete_stale_probe_results_creates_schema_safely();

    std::cout
        << "test_epgsearch_native_fuzzy_stale_probe_administration_service passed"
        << std::endl;

    return 0;
}

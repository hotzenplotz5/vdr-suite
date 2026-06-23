#include "Database.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityFreshnessPolicy.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyStaleProbeAdministrationController.h"
#include "EpgSearchNativeFuzzyStaleProbeAdministrationService.h"

#include <cassert>
#include <cstdio>
#include <iostream>

static const char* dbPath =
    "/tmp/vdr-suite-native-fuzzy-stale-probe-admin-controller-test.db";

static Database openDatabase()
{
    std::remove(dbPath);

    Database database;
    assert(database.open(dbPath));
    assert(database.isOpen());

    return database;
}

static void createFreshAndStaleProbeRows(
    Database& database,
    EpgSearchNativeFuzzyCapabilityRepository& repository)
{
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
}

static void test_get_stale_probe_results()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy freshnessPolicy(60);
    EpgSearchNativeFuzzyStaleProbeAdministrationService service(
        repository,
        freshnessPolicy);
    EpgSearchNativeFuzzyStaleProbeAdministrationController controller(service);

    createFreshAndStaleProbeRows(database, repository);

    const auto response = controller.getStaleProbeResults();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"staleProbes\"") != std::string::npos);
    assert(response.body.find("\"backendId\":\"stale\"") != std::string::npos);
    assert(response.body.find("\"backendId\":\"fresh\"") == std::string::npos);
}

static void test_delete_stale_probe_results()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy freshnessPolicy(60);
    EpgSearchNativeFuzzyStaleProbeAdministrationService service(
        repository,
        freshnessPolicy);
    EpgSearchNativeFuzzyStaleProbeAdministrationController controller(service);

    createFreshAndStaleProbeRows(database, repository);

    const auto response = controller.deleteStaleProbeResults();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"persistedResultsSeen\":2") != std::string::npos);
    assert(response.body.find("\"staleResultsFound\":1") != std::string::npos);
    assert(response.body.find("\"deletedResults\":1") != std::string::npos);
    assert(response.body.find("\"deleteFailures\":0") != std::string::npos);

    assert(repository.loadPersistedProbeResult("fresh").has_value());
    assert(!repository.loadPersistedProbeResult("stale").has_value());
}

int main()
{
    test_get_stale_probe_results();
    test_delete_stale_probe_results();

    std::cout
        << "test_epgsearch_native_fuzzy_stale_probe_administration_controller passed"
        << std::endl;

    return 0;
}

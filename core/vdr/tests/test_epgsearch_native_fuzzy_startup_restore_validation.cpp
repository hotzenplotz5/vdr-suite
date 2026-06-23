#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityFreshnessPolicy.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyStartupRestoreService.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <cstdio>
#include <iostream>

namespace
{
const char* databasePath =
    "/tmp/vdr-suite-test-epgsearch-native-fuzzy-startup-restore-validation.db";

BackendNode makeDefaultBackend()
{
    BackendNode backend;
    backend.backendId = "default";
    backend.backendName = "Default VDR";
    backend.backendType = "restfulapi";
    backend.enabled = true;
    backend.online = true;
    backend.capabilities = VdrCapabilitySet::snapshotReadOnly();
    return backend;
}

void cleanupDatabase()
{
    std::remove(databasePath);
}

void successfulPersistedProbeRestoresNativeCapability()
{
    cleanupDatabase();

    Database database;
    assert(database.open(databasePath));

    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    assert(repository.ensureSchema());

    assert(
        repository.save(
            "default",
            EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));

    BackendRegistry registry;
    registry.addBackend(makeDefaultBackend());

    BackendRegistryService backendRegistryService(registry);
    EpgSearchNativeFuzzyCapabilityDetector detector;
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy freshnessPolicy;

    EpgSearchNativeFuzzyStartupRestoreService restoreService(
        repository,
        detector,
        backendRegistryService,
        freshnessPolicy);

    const auto summary =
        restoreService.restoreAllBackends();

    assert(summary.schemaReady);
    assert(summary.backendsSeen == 1);
    assert(summary.persistedResultsFound == 1);
    assert(summary.backendsUpdated == 1);
    assert(summary.nativeFuzzyAvailable == 1);
    assert(summary.staleResultsIgnored == 0);

    const auto backend =
        backendRegistryService.getBackend("default");

    assert(backend.has_value());
    assert(backend->capabilities.epgSearchFuzzyNative);

    database.close();
    cleanupDatabase();
}

void missingPersistedProbeDoesNotEnableNativeCapability()
{
    cleanupDatabase();

    Database database;
    assert(database.open(databasePath));

    EpgSearchNativeFuzzyCapabilityRepository repository(database);

    BackendRegistry registry;
    registry.addBackend(makeDefaultBackend());

    BackendRegistryService backendRegistryService(registry);
    EpgSearchNativeFuzzyCapabilityDetector detector;
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy freshnessPolicy;

    EpgSearchNativeFuzzyStartupRestoreService restoreService(
        repository,
        detector,
        backendRegistryService,
        freshnessPolicy);

    const auto summary =
        restoreService.restoreAllBackends();

    assert(summary.schemaReady);
    assert(summary.backendsSeen == 1);
    assert(summary.persistedResultsFound == 0);
    assert(summary.backendsUpdated == 0);
    assert(summary.nativeFuzzyAvailable == 0);
    assert(summary.staleResultsIgnored == 0);

    const auto backend =
        backendRegistryService.getBackend("default");

    assert(backend.has_value());
    assert(!backend->capabilities.epgSearchFuzzyNative);

    database.close();
    cleanupDatabase();
}
}

int main()
{
    successfulPersistedProbeRestoresNativeCapability();
    missingPersistedProbeDoesNotEnableNativeCapability();

    std::cout
        << "test_epgsearch_native_fuzzy_startup_restore_validation passed"
        << std::endl;

    return 0;
}

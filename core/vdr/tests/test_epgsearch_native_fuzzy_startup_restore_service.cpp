#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "CapabilityResolver.h"
#include "Database.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyStartupRestoreService.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <cstdio>
#include <iostream>

static const char* dbPath = "/tmp/vdr-suite-native-fuzzy-startup-restore-test.db";

static Database openDatabase()
{
    std::remove(dbPath);

    Database database;
    assert(database.open(dbPath));
    assert(database.isOpen());

    return database;
}

static BackendNode makeBackend(
    const std::string& backendId)
{
    BackendNode backend;
    backend.backendId = backendId;
    backend.backendName = backendId + " VDR";
    backend.backendType = "restfulapi";
    backend.capabilities = VdrCapabilitySet::snapshotReadOnly();
    backend.enabled = true;
    backend.online = true;

    return backend;
}

static void assertNativeFuzzy(
    const BackendNode& backend,
    bool expected)
{
    CapabilityResolver resolver(backend.capabilities);

    assert(resolver.supports("snapshot.read"));
    assert(resolver.supports("epg.search.fuzzy.fallback"));
    assert(resolver.supports("epg.search.fuzzy.native") == expected);
}

static void test_startup_restore_creates_schema_without_running_probe()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    BackendRegistry registry;
    registry.addBackend(makeBackend("default"));
    BackendRegistryService backendRegistryService(registry);

    EpgSearchNativeFuzzyStartupRestoreService startupRestoreService(
        repository,
        detector,
        backendRegistryService);

    assert(!database.tableExists("epgsearch_native_fuzzy_capability_probes"));

    const auto summary = startupRestoreService.restoreAllBackends();

    assert(summary.schemaReady);
    assert(database.tableExists("epgsearch_native_fuzzy_capability_probes"));
    assert(summary.backendsSeen == 1);
    assert(summary.persistedResultsFound == 0);
    assert(summary.backendsUpdated == 0);
    assert(summary.nativeFuzzyAvailable == 0);

    const auto backend = backendRegistryService.getBackend("default");
    assert(backend.has_value());
    assertNativeFuzzy(*backend, false);
}

static void test_startup_restore_applies_successful_persisted_probe()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());
    assert(repository.save(
        "default",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));

    BackendRegistry registry;
    registry.addBackend(makeBackend("default"));
    BackendRegistryService backendRegistryService(registry);

    EpgSearchNativeFuzzyStartupRestoreService startupRestoreService(
        repository,
        detector,
        backendRegistryService);

    const auto summary = startupRestoreService.restoreAllBackends();

    assert(summary.schemaReady);
    assert(summary.backendsSeen == 1);
    assert(summary.persistedResultsFound == 1);
    assert(summary.backendsUpdated == 1);
    assert(summary.nativeFuzzyAvailable == 1);

    const auto backend = backendRegistryService.getBackend("default");
    assert(backend.has_value());
    assertNativeFuzzy(*backend, true);
}

static void test_startup_restore_applies_failed_persisted_probe()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());

    EpgSearchNativeFuzzyCapabilityProbeResult failed =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();
    failed.modePreserved = false;

    assert(repository.save("default", failed));

    BackendRegistry registry;
    BackendNode backend = makeBackend("default");
    backend.capabilities.epgSearchFuzzyNative = true;
    registry.addBackend(backend);
    BackendRegistryService backendRegistryService(registry);

    EpgSearchNativeFuzzyStartupRestoreService startupRestoreService(
        repository,
        detector,
        backendRegistryService);

    const auto summary = startupRestoreService.restoreAllBackends();

    assert(summary.schemaReady);
    assert(summary.backendsSeen == 1);
    assert(summary.persistedResultsFound == 1);
    assert(summary.backendsUpdated == 1);
    assert(summary.nativeFuzzyAvailable == 0);

    const auto restoredBackend = backendRegistryService.getBackend("default");
    assert(restoredBackend.has_value());
    assertNativeFuzzy(*restoredBackend, false);
}

static void test_startup_restore_is_backend_scoped_and_does_not_create_missing_backend()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());

    EpgSearchNativeFuzzyCapabilityProbeResult failed =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();
    failed.cleanupSucceeded = false;

    assert(repository.save(
        "living-room",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));
    assert(repository.save("holiday-home", failed));
    assert(repository.save(
        "missing",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));

    BackendRegistry registry;
    registry.addBackend(makeBackend("living-room"));
    registry.addBackend(makeBackend("holiday-home"));
    BackendRegistryService backendRegistryService(registry);

    EpgSearchNativeFuzzyStartupRestoreService startupRestoreService(
        repository,
        detector,
        backendRegistryService);

    const auto summary = startupRestoreService.restoreAllBackends();

    assert(summary.schemaReady);
    assert(summary.backendsSeen == 2);
    assert(summary.persistedResultsFound == 2);
    assert(summary.backendsUpdated == 2);
    assert(summary.nativeFuzzyAvailable == 1);
    assert(!backendRegistryService.hasBackend("missing"));

    const auto livingRoom = backendRegistryService.getBackend("living-room");
    const auto holidayHome = backendRegistryService.getBackend("holiday-home");

    assert(livingRoom.has_value());
    assert(holidayHome.has_value());
    assertNativeFuzzy(*livingRoom, true);
    assertNativeFuzzy(*holidayHome, false);
}

int main()
{
    test_startup_restore_creates_schema_without_running_probe();
    test_startup_restore_applies_successful_persisted_probe();
    test_startup_restore_applies_failed_persisted_probe();
    test_startup_restore_is_backend_scoped_and_does_not_create_missing_backend();

    std::cout
        << "test_epgsearch_native_fuzzy_startup_restore_service passed"
        << std::endl;

    return 0;
}

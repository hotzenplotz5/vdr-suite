#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "CapabilityResolver.h"
#include "Database.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyCapabilityRestoreService.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <cstdio>
#include <iostream>

static const char* dbPath = "/tmp/vdr-suite-native-fuzzy-capability-restore-test.db";

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

    assert(resolver.supports("epg.search.fuzzy.fallback"));
    assert(resolver.supports("epg.search.fuzzy.native") == expected);
}

static void test_successful_persisted_probe_restores_native_capability()
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

    EpgSearchNativeFuzzyCapabilityRestoreService restoreService(
        repository,
        detector,
        backendRegistryService);

    const auto beforeRestore = backendRegistryService.getBackend("default");
    assert(beforeRestore.has_value());
    assertNativeFuzzy(*beforeRestore, false);

    const auto result = restoreService.restoreBackend("default");
    assert(result.persistedResultFound);
    assert(result.backendFound);
    assert(result.backendUpdated);
    assert(result.nativeFuzzyAvailable);

    const auto afterRestore = backendRegistryService.getBackend("default");
    assert(afterRestore.has_value());
    assertNativeFuzzy(*afterRestore, true);
}

static void test_failed_persisted_probe_restores_disabled_native_capability()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());

    EpgSearchNativeFuzzyCapabilityProbeResult failed =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();
    failed.tolerancePreserved = false;

    assert(repository.save("default", failed));

    BackendRegistry registry;
    BackendNode backend = makeBackend("default");
    backend.capabilities.epgSearchFuzzyNative = true;
    registry.addBackend(backend);

    BackendRegistryService backendRegistryService(registry);
    EpgSearchNativeFuzzyCapabilityRestoreService restoreService(
        repository,
        detector,
        backendRegistryService);

    const auto result = restoreService.restoreBackend("default");
    assert(result.persistedResultFound);
    assert(result.backendFound);
    assert(result.backendUpdated);
    assert(!result.nativeFuzzyAvailable);

    const auto afterRestore = backendRegistryService.getBackend("default");
    assert(afterRestore.has_value());
    assertNativeFuzzy(*afterRestore, false);
}

static void test_missing_persisted_probe_does_not_update_backend()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());

    BackendRegistry registry;
    registry.addBackend(makeBackend("default"));
    BackendRegistryService backendRegistryService(registry);

    EpgSearchNativeFuzzyCapabilityRestoreService restoreService(
        repository,
        detector,
        backendRegistryService);

    const auto result = restoreService.restoreBackend("default");
    assert(!result.persistedResultFound);
    assert(!result.backendFound);
    assert(!result.backendUpdated);
    assert(!result.nativeFuzzyAvailable);

    const auto backend = backendRegistryService.getBackend("default");
    assert(backend.has_value());
    assertNativeFuzzy(*backend, false);
}

static void test_missing_backend_does_not_get_created_from_persisted_probe()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());
    assert(repository.save(
        "missing",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));

    BackendRegistry registry;
    BackendRegistryService backendRegistryService(registry);

    EpgSearchNativeFuzzyCapabilityRestoreService restoreService(
        repository,
        detector,
        backendRegistryService);

    const auto result = restoreService.restoreBackend("missing");
    assert(result.persistedResultFound);
    assert(!result.backendFound);
    assert(!result.backendUpdated);
    assert(result.nativeFuzzyAvailable);
    assert(!backendRegistryService.hasBackend("missing"));
    assert(backendRegistryService.listBackends().empty());
}

static void test_restore_is_backend_scoped()
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

    BackendRegistry registry;
    registry.addBackend(makeBackend("living-room"));
    registry.addBackend(makeBackend("holiday-home"));

    BackendRegistryService backendRegistryService(registry);
    EpgSearchNativeFuzzyCapabilityRestoreService restoreService(
        repository,
        detector,
        backendRegistryService);

    const auto livingRoomResult = restoreService.restoreBackend("living-room");
    const auto holidayHomeResult = restoreService.restoreBackend("holiday-home");

    assert(livingRoomResult.backendUpdated);
    assert(livingRoomResult.nativeFuzzyAvailable);
    assert(holidayHomeResult.backendUpdated);
    assert(!holidayHomeResult.nativeFuzzyAvailable);

    const auto livingRoom = backendRegistryService.getBackend("living-room");
    const auto holidayHome = backendRegistryService.getBackend("holiday-home");

    assert(livingRoom.has_value());
    assert(holidayHome.has_value());
    assertNativeFuzzy(*livingRoom, true);
    assertNativeFuzzy(*holidayHome, false);
}

int main()
{
    test_successful_persisted_probe_restores_native_capability();
    test_failed_persisted_probe_restores_disabled_native_capability();
    test_missing_persisted_probe_does_not_update_backend();
    test_missing_backend_does_not_get_created_from_persisted_probe();
    test_restore_is_backend_scoped();

    std::cout
        << "test_epgsearch_native_fuzzy_capability_restore_service passed"
        << std::endl;

    return 0;
}

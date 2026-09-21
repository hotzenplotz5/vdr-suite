#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyOperatorRefreshService.h"
#include "ISearchTimerCommandExecutor.h"
#include "ISearchTimerDataSource.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"
#include "SearchTimerService.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

static const char* dbPath =
    "/tmp/vdr-suite-native-fuzzy-operator-refresh-test.db";

class FakeSearchTimerRuntime
    : public ISearchTimerCommandExecutor,
      public ISearchTimerDataSource
{
public:
    bool failCreate = false;
    bool failDelete = false;
    int readbackMode = 5;
    int readbackTolerance = 2;
    bool omitReadback = false;
    std::vector<SearchTimer> timers;

    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        if (failCreate)
        {
            return SearchTimerCreateResult::failed(
                "fake create failed",
                {"fake create failure"});
        }

        SearchTimer timer = SearchTimer::create(
            SearchTimerId::fromBackendNativeId(
                request.backendId,
                "42"),
            request.name,
            request.query,
            SearchTimerState::Active);
        timer.matchOptions().setMode(readbackMode);
        timer.matchOptions().setTolerance(readbackTolerance);

        if (!omitReadback)
        {
            timers.push_back(timer);
        }

        return SearchTimerCreateResult::ok(
            timer,
            "fake create ok");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        (void)request;
        return SearchTimerUpdateResult::failed(
            "not implemented",
            {"not implemented"});
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        if (failDelete)
        {
            return SearchTimerDeleteResult::failed(
                "fake delete failed",
                {"fake delete failure"});
        }

        timers.erase(
            std::remove_if(
                timers.begin(),
                timers.end(),
                [&](const SearchTimer& timer) {
                    return timer.backendNativeId() == request.backendNativeId;
                }),
            timers.end());

        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "fake delete ok");
    }

    SearchTimerResult list(
        const SearchTimerQuery& query) const override
    {
        SearchTimerService service;
        return service.list(
            timers,
            query);
    }
};

static Database openDatabase()
{
    std::remove(dbPath);

    Database database;
    assert(database.open(dbPath));
    assert(database.isOpen());

    return database;
}

static BackendRegistryService createBackendRegistryService(
    BackendRegistry& registry)
{
    BackendNode backend;
    backend.backendId = "default";
    backend.backendName = "Default VDR";
    backend.backendType = "restfulapi";
    backend.enabled = true;
    backend.online = true;
    backend.capabilities = VdrCapabilitySet::snapshotReadOnly();
    registry.addBackend(backend);

    return BackendRegistryService(registry);
}

static void test_successful_operator_refresh_persists_and_updates_capability()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;
    BackendRegistry registry;
    BackendRegistryService registryService = createBackendRegistryService(registry);
    FakeSearchTimerRuntime runtime;

    EpgSearchNativeFuzzyOperatorRefreshService service(
        runtime,
        runtime,
        repository,
        detector,
        registryService);

    EpgSearchNativeFuzzyOperatorRefreshRequest request;
    request.backendId = "default";
    request.probeQuery = "operator refresh probe";
    request.tolerance = 2;

    const auto summary = service.refresh(request);

    assert(summary.status == "refreshed-native-available");
    assert(summary.backendKnown);
    assert(summary.createAttempted);
    assert(summary.createAccepted);
    assert(summary.readbackAvailable);
    assert(summary.modePreserved);
    assert(summary.tolerancePreserved);
    assert(summary.cleanupAttempted);
    assert(summary.cleanupSucceeded);
    assert(summary.persisted);
    assert(summary.backendCapabilitiesUpdated);
    assert(summary.nativeFuzzyAvailable);
    assert(runtime.timers.empty());

    const auto persisted = repository.loadPersistedProbeResult("default");
    assert(persisted.has_value());
    assert(detector.nativeFuzzyAvailable(persisted->probeResult));

    const auto backend = registryService.getBackend("default");
    assert(backend.has_value());
    assert(backend->capabilities.epgSearchFuzzyNative);
}

static void test_mode_not_preserved_disables_native_capability()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;
    BackendRegistry registry;
    BackendRegistryService registryService = createBackendRegistryService(registry);
    FakeSearchTimerRuntime runtime;
    runtime.readbackMode = 0;

    EpgSearchNativeFuzzyOperatorRefreshService service(
        runtime,
        runtime,
        repository,
        detector,
        registryService);

    EpgSearchNativeFuzzyOperatorRefreshRequest request;
    request.probeQuery = "operator refresh mode downgrade";

    const auto summary = service.refresh(request);

    assert(summary.status == "mode-not-preserved");
    assert(summary.createAccepted);
    assert(summary.readbackAvailable);
    assert(!summary.modePreserved);
    assert(summary.cleanupSucceeded);
    assert(summary.persisted);
    assert(!summary.nativeFuzzyAvailable);

    const auto backend = registryService.getBackend("default");
    assert(backend.has_value());
    assert(!backend->capabilities.epgSearchFuzzyNative);
}

static void test_missing_backend_does_not_create_probe_timer()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;
    BackendRegistry registry;
    BackendRegistryService registryService(registry);
    FakeSearchTimerRuntime runtime;

    EpgSearchNativeFuzzyOperatorRefreshService service(
        runtime,
        runtime,
        repository,
        detector,
        registryService);

    EpgSearchNativeFuzzyOperatorRefreshRequest request;
    request.backendId = "missing";

    const auto summary = service.refresh(request);

    assert(summary.status == "backend-not-found");
    assert(!summary.backendKnown);
    assert(!summary.createAttempted);
    assert(!summary.persisted);
    assert(runtime.timers.empty());
}

int main()
{
    test_successful_operator_refresh_persists_and_updates_capability();
    test_mode_not_preserved_disables_native_capability();
    test_missing_backend_does_not_create_probe_timer();

    std::cout
        << "test_epgsearch_native_fuzzy_operator_refresh_service passed"
        << std::endl;

    return 0;
}

#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyOperatorRefreshController.h"
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
    "/tmp/vdr-suite-native-fuzzy-operator-refresh-controller-test.db";

class FakeSearchTimerRuntime
    : public ISearchTimerCommandExecutor,
      public ISearchTimerDataSource
{
public:
    bool failCreate = false;
    int readbackMode = 5;
    int readbackTolerance = 2;
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
        timers.push_back(timer);

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

static void test_refresh_api_with_empty_body_uses_defaults()
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
    EpgSearchNativeFuzzyOperatorRefreshController controller(service);

    const auto response = controller.refreshBody("");

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"status\":\"refreshed-native-available\"") != std::string::npos);
    assert(response.body.find("\"nativeFuzzyAvailable\":true") != std::string::npos);
    assert(response.body.find("\"backendCapabilitiesUpdated\":true") != std::string::npos);
    assert(runtime.timers.empty());
}

static void test_refresh_api_parses_backend_query_and_tolerance()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;
    BackendRegistry registry;
    BackendRegistryService registryService = createBackendRegistryService(registry);
    FakeSearchTimerRuntime runtime;
    runtime.readbackTolerance = 3;

    EpgSearchNativeFuzzyOperatorRefreshService service(
        runtime,
        runtime,
        repository,
        detector,
        registryService);
    EpgSearchNativeFuzzyOperatorRefreshController controller(service);

    const auto response = controller.refreshBody(
        "{\"backend\":\"default\",\"query\":\"operator api probe\",\"tolerance\":3}");

    assert(response.statusCode == 200);
    assert(response.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(response.body.find("\"probeQuery\":\"operator api probe\"") != std::string::npos);
    assert(response.body.find("\"tolerance\":3") != std::string::npos);
    assert(response.body.find("\"tolerancePreserved\":true") != std::string::npos);
}

static void test_refresh_api_returns_404_for_missing_backend()
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
    EpgSearchNativeFuzzyOperatorRefreshController controller(service);

    const auto response = controller.refreshBody(
        "{\"backendId\":\"missing\"}");

    assert(response.statusCode == 404);
    assert(response.body.find("\"status\":\"backend-not-found\"") != std::string::npos);
    assert(response.body.find("backend not found: missing") != std::string::npos);
    assert(runtime.timers.empty());
}

int main()
{
    test_refresh_api_with_empty_body_uses_defaults();
    test_refresh_api_parses_backend_query_and_tolerance();
    test_refresh_api_returns_404_for_missing_backend();

    std::cout
        << "test_epgsearch_native_fuzzy_operator_refresh_controller passed"
        << std::endl;

    return 0;
}

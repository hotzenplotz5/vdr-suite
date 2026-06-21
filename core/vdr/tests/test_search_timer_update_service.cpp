#include "SearchTimerUpdateService.h"

#include <cassert>
#include <iostream>

class TestSearchTimerCommandExecutor final : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++createCallCount_;

        return SearchTimerCreateResult::ok(
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    request.backendId,
                    "created-searchtimer-1"),
                request.name,
                request.query,
                request.active ? SearchTimerState::Active : SearchTimerState::Inactive),
            "searchtimer created");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        ++updateCallCount_;

        return SearchTimerUpdateResult::ok(
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    request.backendId,
                    request.backendNativeId),
                request.name,
                request.query,
                request.active ? SearchTimerState::Active : SearchTimerState::Inactive),
            "searchtimer updated");
    }

    int createCallCount() const
    {
        return createCallCount_;
    }

    int updateCallCount() const
    {
        return updateCallCount_;
    }

private:
    int createCallCount_ = 0;
    int updateCallCount_ = 0;
};

static SearchTimerUpdateRequest makeRequest()
{
    SearchTimerUpdateRequest request;
    request.backendId = "home-vdr";
    request.backendNativeId = "searchtimer-42";
    request.name = "Terra X Suche";
    request.query = "Terra X";
    request.active = true;
    return request;
}

static void test_update_delegates_valid_request_to_executor()
{
    SearchTimerUpdateService service;
    TestSearchTimerCommandExecutor executor;

    const SearchTimerUpdateResult result =
        service.update(
            makeRequest(),
            executor);

    assert(result.success == true);
    assert(result.message == "searchtimer updated");
    assert(result.searchTimer.backendId() == "home-vdr");
    assert(result.searchTimer.backendNativeId() == "searchtimer-42");
    assert(result.searchTimer.name() == "Terra X Suche");
    assert(result.searchTimer.query() == "Terra X");
    assert(result.searchTimer.state() == SearchTimerState::Active);
    assert(executor.updateCallCount() == 1);
    assert(executor.createCallCount() == 0);
}

static void test_update_preserves_inactive_state()
{
    SearchTimerUpdateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerUpdateRequest request = makeRequest();
    request.active = false;

    const SearchTimerUpdateResult result =
        service.update(
            request,
            executor);

    assert(result.success == true);
    assert(result.searchTimer.state() == SearchTimerState::Inactive);
    assert(executor.updateCallCount() == 1);
}

static void test_update_requires_backend_id()
{
    SearchTimerUpdateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerUpdateRequest request = makeRequest();
    request.backendId = "";

    const SearchTimerUpdateResult result =
        service.update(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer backend id is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "backendId is required");
    assert(executor.updateCallCount() == 0);
}

static void test_update_requires_backend_native_id()
{
    SearchTimerUpdateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerUpdateRequest request = makeRequest();
    request.backendNativeId = "";

    const SearchTimerUpdateResult result =
        service.update(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer backend native id is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "backendNativeId is required");
    assert(executor.updateCallCount() == 0);
}

static void test_update_requires_name()
{
    SearchTimerUpdateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerUpdateRequest request = makeRequest();
    request.name = "";

    const SearchTimerUpdateResult result =
        service.update(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer name is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "name is required");
    assert(executor.updateCallCount() == 0);
}

static void test_update_requires_query()
{
    SearchTimerUpdateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerUpdateRequest request = makeRequest();
    request.query = "";

    const SearchTimerUpdateResult result =
        service.update(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer query is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "query is required");
    assert(executor.updateCallCount() == 0);
}

int main()
{
    test_update_delegates_valid_request_to_executor();
    test_update_preserves_inactive_state();
    test_update_requires_backend_id();
    test_update_requires_backend_native_id();
    test_update_requires_name();
    test_update_requires_query();

    std::cout
        << "test_search_timer_update_service passed"
        << std::endl;

    return 0;
}

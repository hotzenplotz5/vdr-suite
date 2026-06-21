#include "SearchTimerCreateService.h"

#include <cassert>
#include <iostream>
#include <string>

class TestSearchTimerCommandExecutor final : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++callCount_;

        SearchTimerState state =
            request.active
                ? SearchTimerState::Active
                : SearchTimerState::Inactive;

        return SearchTimerCreateResult::ok(
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    request.backendId,
                    "created-searchtimer-1"),
                request.name,
                request.query,
                state),
            "searchtimer created");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
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


    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "searchtimer deleted");
    }

    int callCount() const
    {
        return callCount_;
    }

private:
    int callCount_ = 0;
};

static SearchTimerCreateRequest makeRequest()
{
    SearchTimerCreateRequest request;
    request.backendId = "home-vdr";
    request.name = "Terra X Suche";
    request.query = "Terra X";
    request.active = true;
    return request;
}

static void test_create_delegates_valid_request_to_executor()
{
    SearchTimerCreateService service;
    TestSearchTimerCommandExecutor executor;

    const SearchTimerCreateResult result =
        service.create(
            makeRequest(),
            executor);

    assert(result.success == true);
    assert(result.message == "searchtimer created");
    assert(result.searchTimer.backendId() == "home-vdr");
    assert(result.searchTimer.backendNativeId() == "created-searchtimer-1");
    assert(result.searchTimer.name() == "Terra X Suche");
    assert(result.searchTimer.query() == "Terra X");
    assert(result.searchTimer.state() == SearchTimerState::Active);
    assert(executor.callCount() == 1);
}

static void test_create_preserves_inactive_state()
{
    SearchTimerCreateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerCreateRequest request = makeRequest();
    request.active = false;

    const SearchTimerCreateResult result =
        service.create(
            request,
            executor);

    assert(result.success == true);
    assert(result.searchTimer.state() == SearchTimerState::Inactive);
    assert(executor.callCount() == 1);
}

static void test_create_requires_backend_id()
{
    SearchTimerCreateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerCreateRequest request = makeRequest();
    request.backendId = "";

    const SearchTimerCreateResult result =
        service.create(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer backend id is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "backendId is required");
    assert(executor.callCount() == 0);
}

static void test_create_requires_name()
{
    SearchTimerCreateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerCreateRequest request = makeRequest();
    request.name = "";

    const SearchTimerCreateResult result =
        service.create(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer name is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "name is required");
    assert(executor.callCount() == 0);
}

static void test_create_requires_query()
{
    SearchTimerCreateService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerCreateRequest request = makeRequest();
    request.query = "";

    const SearchTimerCreateResult result =
        service.create(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer query is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "query is required");
    assert(executor.callCount() == 0);
}

int main()
{
    test_create_delegates_valid_request_to_executor();
    test_create_preserves_inactive_state();
    test_create_requires_backend_id();
    test_create_requires_name();
    test_create_requires_query();

    std::cout
        << "test_search_timer_create_service passed"
        << std::endl;

    return 0;
}

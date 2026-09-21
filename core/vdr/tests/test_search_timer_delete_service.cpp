#include "SearchTimerDeleteService.h"

#include <cassert>
#include <iostream>

class TestSearchTimerCommandExecutor final : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
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
        ++removeCallCount_;

        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "searchtimer deleted");
    }

    int removeCallCount() const
    {
        return removeCallCount_;
    }

private:
    int removeCallCount_ = 0;
};

static SearchTimerDeleteRequest makeRequest()
{
    SearchTimerDeleteRequest request;
    request.backendId = "home-vdr";
    request.backendNativeId = "searchtimer-42";
    return request;
}

static void test_remove_delegates_valid_request_to_executor()
{
    SearchTimerDeleteService service;
    TestSearchTimerCommandExecutor executor;

    const SearchTimerDeleteResult result =
        service.remove(
            makeRequest(),
            executor);

    assert(result.success == true);
    assert(result.message == "searchtimer deleted");
    assert(result.backendId == "home-vdr");
    assert(result.backendNativeId == "searchtimer-42");
    assert(executor.removeCallCount() == 1);
}

static void test_remove_requires_backend_id()
{
    SearchTimerDeleteService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerDeleteRequest request = makeRequest();
    request.backendId = "";

    const SearchTimerDeleteResult result =
        service.remove(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer backend id is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "backendId is required");
    assert(executor.removeCallCount() == 0);
}

static void test_remove_requires_backend_native_id()
{
    SearchTimerDeleteService service;
    TestSearchTimerCommandExecutor executor;
    SearchTimerDeleteRequest request = makeRequest();
    request.backendNativeId = "";

    const SearchTimerDeleteResult result =
        service.remove(
            request,
            executor);

    assert(result.success == false);
    assert(result.message == "searchtimer backend native id is required");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "backendNativeId is required");
    assert(executor.removeCallCount() == 0);
}

int main()
{
    test_remove_delegates_valid_request_to_executor();
    test_remove_requires_backend_id();
    test_remove_requires_backend_native_id();

    std::cout
        << "test_search_timer_delete_service passed"
        << std::endl;

    return 0;
}

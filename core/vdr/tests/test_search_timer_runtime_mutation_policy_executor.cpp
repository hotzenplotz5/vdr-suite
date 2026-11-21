#include "SearchTimerRuntimeMutationPolicyExecutor.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{
class CountingExecutor final : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++createCalls;
        SearchTimer timer =
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    request.backendId,
                    "created-1"),
                request.name,
                request.query,
                SearchTimerState::Active);
        return SearchTimerCreateResult::ok(
            timer,
            "created by delegate");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        ++updateCalls;
        SearchTimer timer =
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    request.backendId,
                    request.backendNativeId),
                request.name,
                request.query,
                SearchTimerState::Active);
        return SearchTimerUpdateResult::ok(
            timer,
            "updated by delegate");
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        ++deleteCalls;
        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "deleted by delegate");
    }

    int createCalls = 0;
    int updateCalls = 0;
    int deleteCalls = 0;
};

SearchTimerCreateRequest makeCreateRequest()
{
    SearchTimerCreateRequest request;
    request.backendId = "default";
    request.name = "TEST-Amerika-title-only";
    request.query = "Amerika";
    request.active = true;
    request.compareTitle = true;
    request.compareSubtitle = false;
    request.compareSummary = false;
    request.compareCategories = false;
    return request;
}

SearchTimerUpdateRequest makeUpdateRequest()
{
    SearchTimerUpdateRequest request;
    request.backendId = "default";
    request.backendNativeId = "17";
    request.name = "TEST-Amerika-title-only";
    request.query = "Amerika";
    request.active = true;
    request.compareTitle = true;
    request.compareSubtitle = false;
    request.compareSummary = false;
    request.compareCategories = false;
    return request;
}

SearchTimerDeleteRequest makeDeleteRequest()
{
    SearchTimerDeleteRequest request;
    request.backendId = "default";
    request.backendNativeId = "17";
    return request;
}

void closedPolicyBlocksCreateUpdateDelete()
{
    CountingExecutor delegate;
    SearchTimerRuntimeMutationPolicyExecutor executor(
        delegate,
        false);

    const auto createResult =
        executor.create(makeCreateRequest());
    const auto updateResult =
        executor.update(makeUpdateRequest());
    const auto deleteResult =
        executor.remove(makeDeleteRequest());

    assert(!executor.mutationAllowed());
    assert(!createResult.success);
    assert(!updateResult.success);
    assert(!deleteResult.success);
    assert(createResult.message == "searchtimer runtime mutation policy gate is closed");
    assert(updateResult.message == "searchtimer runtime mutation policy gate is closed");
    assert(deleteResult.message == "searchtimer runtime mutation policy gate is closed");
    assert(delegate.createCalls == 0);
    assert(delegate.updateCalls == 0);
    assert(delegate.deleteCalls == 0);
}

void openPolicyDelegatesCreateUpdateDelete()
{
    CountingExecutor delegate;
    SearchTimerRuntimeMutationPolicyExecutor executor(
        delegate,
        true);

    const auto createResult =
        executor.create(makeCreateRequest());
    const auto updateResult =
        executor.update(makeUpdateRequest());
    const auto deleteResult =
        executor.remove(makeDeleteRequest());

    assert(executor.mutationAllowed());
    assert(createResult.success);
    assert(updateResult.success);
    assert(deleteResult.success);
    assert(createResult.searchTimer.query() == "Amerika");
    assert(updateResult.searchTimer.backendNativeId() == "17");
    assert(deleteResult.backendNativeId == "17");
    assert(delegate.createCalls == 1);
    assert(delegate.updateCalls == 1);
    assert(delegate.deleteCalls == 1);
}
}

int main()
{
    closedPolicyBlocksCreateUpdateDelete();
    openPolicyDelegatesCreateUpdateDelete();

    std::cout
        << "test_search_timer_runtime_mutation_policy_executor passed"
        << std::endl;

    return 0;
}

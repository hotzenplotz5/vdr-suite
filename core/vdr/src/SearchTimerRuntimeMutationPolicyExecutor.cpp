#include "SearchTimerRuntimeMutationPolicyExecutor.h"

#include <utility>

SearchTimerRuntimeMutationPolicyExecutor::SearchTimerRuntimeMutationPolicyExecutor(
    ISearchTimerCommandExecutor& delegate,
    bool mutationAllowed,
    std::string blockedMessage,
    bool allowBrowserCreate)
    : delegate_(delegate),
      mutationAllowed_(mutationAllowed),
      allowBrowserCreate_(allowBrowserCreate),
      blockedMessage_(std::move(blockedMessage))
{
}

SearchTimerCreateResult SearchTimerRuntimeMutationPolicyExecutor::create(
    const SearchTimerCreateRequest& request)
{
    // Only the existing, authenticated Browser CREATE route can use this
    // separately enabled entry. Legacy UPDATE/DELETE stay closed.
    if (!mutationAllowed_ && !allowBrowserCreate_)
    {
        return SearchTimerCreateResult::failed(
            blockedMessage_,
            {blockedMessage_});
    }

    return delegate_.create(request);
}

SearchTimerUpdateResult SearchTimerRuntimeMutationPolicyExecutor::update(
    const SearchTimerUpdateRequest& request)
{
    if (!mutationAllowed_)
    {
        return SearchTimerUpdateResult::failed(
            blockedMessage_,
            {blockedMessage_});
    }

    return delegate_.update(request);
}

SearchTimerDeleteResult SearchTimerRuntimeMutationPolicyExecutor::remove(
    const SearchTimerDeleteRequest& request)
{
    if (!mutationAllowed_)
    {
        return SearchTimerDeleteResult::failed(
            blockedMessage_,
            {blockedMessage_});
    }

    return delegate_.remove(request);
}

bool SearchTimerRuntimeMutationPolicyExecutor::mutationAllowed() const
{
    return mutationAllowed_;
}

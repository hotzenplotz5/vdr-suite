#include "SearchTimerRuntimeMutationPolicyExecutor.h"

#include <utility>

SearchTimerRuntimeMutationPolicyExecutor::SearchTimerRuntimeMutationPolicyExecutor(
    ISearchTimerCommandExecutor& delegate,
    bool mutationAllowed,
    std::string blockedMessage)
    : delegate_(delegate),
      mutationAllowed_(mutationAllowed),
      blockedMessage_(std::move(blockedMessage))
{
}

SearchTimerCreateResult SearchTimerRuntimeMutationPolicyExecutor::create(
    const SearchTimerCreateRequest& request)
{
    if (!mutationAllowed_)
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

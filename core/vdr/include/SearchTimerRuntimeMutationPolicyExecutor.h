#pragma once

#include "ISearchTimerCommandExecutor.h"

#include <string>

class SearchTimerRuntimeMutationPolicyExecutor final
    : public ISearchTimerCommandExecutor
{
public:
    SearchTimerRuntimeMutationPolicyExecutor(
        ISearchTimerCommandExecutor& delegate,
        bool mutationAllowed,
        std::string blockedMessage =
            "searchtimer runtime mutation policy gate is closed");

    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override;

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override;

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override;

    bool mutationAllowed() const;

private:
    ISearchTimerCommandExecutor& delegate_;
    bool mutationAllowed_;
    std::string blockedMessage_;
};

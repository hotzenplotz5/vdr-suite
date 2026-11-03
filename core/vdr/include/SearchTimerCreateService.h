#pragma once

#include "ISearchTimerCommandExecutor.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerCreateResult.h"

#include <string>
#include <utility>

class SearchTimerCreateService
{
public:
    explicit SearchTimerCreateService(std::string providerBackendId = "")
        : providerBackendId_(std::move(providerBackendId)) {}

    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request,
        ISearchTimerCommandExecutor& executor) const;

private:
    std::string providerBackendId_;
};

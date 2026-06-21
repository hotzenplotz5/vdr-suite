#pragma once

#include "ISearchTimerCommandExecutor.h"

class IHttpClient;

class RestfulApiSearchTimerCommandExecutor final : public ISearchTimerCommandExecutor
{
public:
    explicit RestfulApiSearchTimerCommandExecutor(
        IHttpClient& httpClient);

    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override;

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override;

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override;

private:
    IHttpClient& httpClient_;
};

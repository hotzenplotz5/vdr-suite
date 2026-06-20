#pragma once

#include "IHttpClient.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"

#include <string>

class RestfulApiSearchTimerAdapter {
public:
    RestfulApiSearchTimerAdapter(
        std::string backendId,
        IHttpClient& httpClient);

    SearchTimerResult list(
        const SearchTimerQuery& query) const;

private:
    std::string backendId_;
    IHttpClient& httpClient_;
};
#pragma once

#include "IHttpClient.h"
#include "ISearchTimerDataSource.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"

#include <string>

class RestfulApiSearchTimerAdapter : public ISearchTimerDataSource {
public:
    RestfulApiSearchTimerAdapter(
        std::string backendId,
        IHttpClient& httpClient);

    SearchTimerResult list(
        const SearchTimerQuery& query) const override;

private:
    std::string backendId_;
    IHttpClient& httpClient_;
};
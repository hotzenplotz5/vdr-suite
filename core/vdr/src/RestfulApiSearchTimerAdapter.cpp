#include "RestfulApiSearchTimerAdapter.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RestfulApiSearchTimerMapper.h"
#include "SearchTimerService.h"

#include <utility>
#include <vector>

RestfulApiSearchTimerAdapter::RestfulApiSearchTimerAdapter(
    std::string backendId,
    IHttpClient& httpClient)
    : backendId_(std::move(backendId)),
      httpClient_(httpClient)
{
}

SearchTimerResult RestfulApiSearchTimerAdapter::list(
    const SearchTimerQuery& query) const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/searchtimers.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    if (response.statusCode != 200) {
        return SearchTimerResult::empty(
            query.limit(),
            query.offset());
    }

    std::vector<SearchTimer> timers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            backendId_,
            response.body);

    SearchTimerService service;
    return service.list(
        timers,
        query);
}
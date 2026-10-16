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
        // Do not turn a provider rejection or outage into a genuine
        // empty SearchTimer list. A Public-v1 client must fail closed.
        return SearchTimerResult::unavailable(
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
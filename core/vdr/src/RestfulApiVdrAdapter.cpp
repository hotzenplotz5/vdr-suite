#include "RestfulApiVdrAdapter.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RestfulApiEventMapper.h"
#include "RestfulApiStatusMapper.h"

RestfulApiVdrAdapter::RestfulApiVdrAdapter(VdrConfig config, IHttpClient& httpClient)
    : config_(config),
      httpClient_(httpClient)
{
}

VdrStatus RestfulApiVdrAdapter::getStatus() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/info.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    return RestfulApiStatusMapper::parseStatus(response.body, config_, response.statusCode);
}

std::vector<VdrEvent> RestfulApiVdrAdapter::getEvents() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/events.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    if (response.statusCode != 200) {
        return {};
    }

    return RestfulApiEventMapper::parseEvents(response.body);
}

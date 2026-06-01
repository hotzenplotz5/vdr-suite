#include "RestfulApiVdrAdapter.h"

#include "HttpRequest.h"
#include "HttpResponse.h"

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

    VdrStatus status;
    status.enabled = config_.enabled;
    status.mode = config_.mode;
    status.host = config_.host;
    status.port = config_.port;
    status.state = response.statusCode == 200 ? "restfulapi" : "error";

    return status;
}

std::vector<VdrEvent> RestfulApiVdrAdapter::getEvents() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/events.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);
    (void)response;

    return {};
}

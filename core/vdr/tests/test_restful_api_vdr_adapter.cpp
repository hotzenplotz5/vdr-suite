#include "MockHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrStatus.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

int main()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = "127.0.0.1";
    config.port = 8002;

    MockHttpClient httpClient;

    HttpResponse okResponse;
    okResponse.statusCode = 200;
    okResponse.headers["Content-Type"] = "application/json";
    okResponse.body = "{\"status\":\"ok\"}";

    httpClient.setResponse(okResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);

    VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "restfulapi");
    assert(status.host == "127.0.0.1");
    assert(status.port == 8002);
    assert(status.state == "restfulapi");

    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/info.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");

    HttpResponse errorResponse;
    errorResponse.statusCode = 500;
    errorResponse.body = "error";

    httpClient.setResponse(errorResponse);

    VdrStatus errorStatus = adapter.getStatus();

    assert(errorStatus.enabled == true);
    assert(errorStatus.mode == "restfulapi");
    assert(errorStatus.host == "127.0.0.1");
    assert(errorStatus.port == 8002);
    assert(errorStatus.state == "error");

    assert(httpClient.requestCount() == 2);

    HttpResponse eventsResponse;
    eventsResponse.statusCode = 200;
    eventsResponse.headers["Content-Type"] = "application/json";
    eventsResponse.body = "{\"events\":[]}";

    httpClient.setResponse(eventsResponse);

    std::vector<VdrEvent> events = adapter.getEvents();

    assert(events.empty() == true);
    assert(httpClient.requestCount() == 3);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/events.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");

    std::unique_ptr<IVdrAdapter> interfaceAdapter =
        std::make_unique<RestfulApiVdrAdapter>(config, httpClient);

    VdrStatus interfaceStatus = interfaceAdapter->getStatus();

    assert(interfaceStatus.mode == "restfulapi");

    return 0;
}

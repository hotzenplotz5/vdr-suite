#include "BasicHttpClient.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrConfig.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string envOrDefault(const char* name, const std::string& fallback)
{
    const char* value = std::getenv(name);
    return value == nullptr || std::string(value).empty()
        ? fallback
        : std::string(value);
}

int envPortOrDefault(const char* name, int fallback)
{
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

HttpResponse getJson(BasicHttpClient& httpClient, const std::string& url)
{
    HttpRequest request;
    request.method = "GET";
    request.url = url;
    request.headers["Accept"] = "application/json";

    return httpClient.execute(request);
}

void assertHttpOk(BasicHttpClient& httpClient, const std::string& url)
{
    HttpResponse response = getJson(httpClient, url);

    if (response.statusCode != 200) {
        std::cerr
            << "Expected HTTP 200 for "
            << url
            << " but got "
            << response.statusCode
            << std::endl;
    }

    assert(response.statusCode == 200);
    assert(!response.body.empty());
}

void assertContains(const std::string& body, const std::string& token)
{
    if (body.find(token) == std::string::npos) {
        std::cerr
            << "Expected response to contain token: "
            << token
            << std::endl
            << body
            << std::endl;
    }

    assert(body.find(token) != std::string::npos);
}

} // namespace

int main()
{
    const std::string host = envOrDefault("VDR_SUITE_LOCAL_RESTFULAPI_HOST", "127.0.0.1");
    const int port = envPortOrDefault("VDR_SUITE_LOCAL_RESTFULAPI_PORT", 8002);

    std::cout
        << "Running local RESTfulAPI integration test against "
        << host
        << ":"
        << port
        << std::endl;

    BasicHttpClient httpClient(host, port);

    assertHttpOk(httpClient, "/info.json");
    assertHttpOk(httpClient, "/channels.json");
    assertHttpOk(httpClient, "/events.json");
    assertHttpOk(httpClient, "/recordings.json");
    assertHttpOk(httpClient, "/timers.json");

    HttpResponse changeStateResponse = getJson(httpClient, "/change-state.json");

    if (changeStateResponse.statusCode != 200) {
        std::cerr
            << "Expected HTTP 200 for /change-state.json but got "
            << changeStateResponse.statusCode
            << std::endl;
    }

    assert(changeStateResponse.statusCode == 200);
    assertContains(changeStateResponse.body, "statusVersion");
    assertContains(changeStateResponse.body, "channelsVersion");
    assertContains(changeStateResponse.body, "recordingsVersion");
    assertContains(changeStateResponse.body, "timersVersion");
    assertContains(changeStateResponse.body, "eventsVersion");

    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = host;
    config.port = port;

    RestfulApiVdrAdapter adapter(config, httpClient);

    VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "restfulapi");
    assert(status.host == host);
    assert(status.port == port);
    assert(status.state == "restfulapi");

    std::vector<VdrChannel> channels = adapter.getChannels();
    VdrChangeState changeState = adapter.getChangeState();

    assert(!channels.empty());

    std::cout
        << "Local RESTfulAPI integration test passed"
        << std::endl
        << "Channels: "
        << channels.size()
        << std::endl
        << "Change state: "
        << "status="
        << changeState.statusVersion
        << ", channels="
        << changeState.channelsVersion
        << ", recordings="
        << changeState.recordingsVersion
        << ", timers="
        << changeState.timersVersion
        << ", events="
        << changeState.eventsVersion
        << std::endl;

    return 0;
}

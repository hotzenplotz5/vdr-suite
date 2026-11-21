#include "EpgQueryService.h"
#include "MockHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrService.h"

#include <cassert>
#include <iostream>

static VdrConfig make_restfulapi_config()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = "127.0.0.1";
    config.port = 8002;
    return config;
}

static HttpResponse make_empty_events_response()
{
    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"events\":[]}";
    return response;
}

static void test_now_next_generates_selective_restfulapi_url()
{
    MockHttpClient httpClient;
    httpClient.setResponse(make_empty_events_response());

    RestfulApiVdrAdapter adapter(make_restfulapi_config(), httpClient);
    VdrService vdrService(adapter);
    EpgQueryService epgService(vdrService);

    const auto events =
        epgService.getNowNext("C-61441-10023-10376", 1780450000);

    assert(events.empty() == true);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/events/C-61441-10023-10376.json?from=1780450000&timespan=7200&chevents=2");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
}

static void test_time_window_generates_selective_restfulapi_url()
{
    MockHttpClient httpClient;
    httpClient.setResponse(make_empty_events_response());

    RestfulApiVdrAdapter adapter(make_restfulapi_config(), httpClient);
    VdrService vdrService(adapter);
    EpgQueryService epgService(vdrService);

    const auto events =
        epgService.getTimeWindow(
            "C-61441-10023-10376",
            1780450000,
            14400);

    assert(events.empty() == true);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/events/C-61441-10023-10376.json?from=1780450000&timespan=14400");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
}

static void test_channel_window_generates_selective_restfulapi_url()
{
    MockHttpClient httpClient;
    httpClient.setResponse(make_empty_events_response());

    RestfulApiVdrAdapter adapter(make_restfulapi_config(), httpClient);
    VdrService vdrService(adapter);
    EpgQueryService epgService(vdrService);

    const auto events =
        epgService.getChannelWindow(
            "C-61441-10023-10376",
            1780450000,
            14400,
            20);

    assert(events.empty() == true);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/events/C-61441-10023-10376.json?from=1780450000&timespan=14400&limit=20");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
}

int main()
{
    test_now_next_generates_selective_restfulapi_url();
    test_time_window_generates_selective_restfulapi_url();
    test_channel_window_generates_selective_restfulapi_url();

    std::cout << "test_epg_query_service_restfulapi passed" << std::endl;
    return 0;
}

#include "MockHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrChangeState.h"
#include "VdrConfig.h"

#include <cassert>

static VdrConfig make_restfulapi_config()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = "127.0.0.1";
    config.port = 8002;
    return config;
}

static void test_restful_api_vdr_adapter_requests_change_state_endpoint()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "application/json";
    response.body =
        "{"
        "\"bootID\":\"654d74f6db4b49fba310b4ba43627617\","
        "\"channelsUpdate\":0,"
        "\"recordingsUpdate\":0,"
        "\"timersUpdate\":0,"
        "\"eventsUpdate\":0"
        "}";

    httpClient.setResponse(response);

    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrChangeState state = adapter.getChangeState();

    assert(state.bootId == "654d74f6db4b49fba310b4ba43627617");
    assert(state.channelsVersion == 0);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/change-state.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
}

static void test_restful_api_vdr_adapter_maps_change_state_response()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "application/json";
    response.body =
        "{"
        "\"bootID\":\"654d74f6db4b49fba310b4ba43627617\","
        "\"channelsUpdate\":893362601689,"
        "\"recordingsUpdate\":893362601690,"
        "\"timersUpdate\":893362601691,"
        "\"eventsUpdate\":893362601692"
        "}";

    httpClient.setResponse(response);

    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrChangeState state = adapter.getChangeState();

    assert(state.bootId == "654d74f6db4b49fba310b4ba43627617");
    assert(state.statusVersion == 0);
    assert(state.channelsVersion == 893362601689LL);
    assert(state.recordingsVersion == 893362601690LL);
    assert(state.timersVersion == 893362601691LL);
    assert(state.eventsVersion == 893362601692LL);
    assert(state.isEmpty() == false);
}

static void test_restful_api_vdr_adapter_ignores_http_error_for_change_state()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 500;
    response.body = "error";

    httpClient.setResponse(response);

    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrChangeState state = adapter.getChangeState();

    assert(state.isEmpty() == true);
}

static void test_restful_api_vdr_adapter_tolerates_invalid_change_state_json()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.body = "not json";

    httpClient.setResponse(response);

    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrChangeState state = adapter.getChangeState();

    assert(state.isEmpty() == true);
}

static void test_restful_api_vdr_adapter_tolerates_incomplete_change_state_json()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"channelsUpdate\":7}";

    httpClient.setResponse(response);

    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrChangeState state = adapter.getChangeState();

    assert(state.bootId == "");
    assert(state.statusVersion == 0);
    assert(state.channelsVersion == 7);
    assert(state.recordingsVersion == 0);
    assert(state.timersVersion == 0);
    assert(state.eventsVersion == 0);
}

int main()
{
    test_restful_api_vdr_adapter_requests_change_state_endpoint();
    test_restful_api_vdr_adapter_maps_change_state_response();
    test_restful_api_vdr_adapter_ignores_http_error_for_change_state();
    test_restful_api_vdr_adapter_tolerates_invalid_change_state_json();
    test_restful_api_vdr_adapter_tolerates_incomplete_change_state_json();

    return 0;
}

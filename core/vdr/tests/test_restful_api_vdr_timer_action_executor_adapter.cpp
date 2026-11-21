#include "RestfulApiVdrTimerActionExecutorAdapter.h"

#include "MockHttpClient.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"

#include <cassert>
#include <memory>

static VdrTimerOperationRequest makeRequest()
{
    VdrTimerOperationRequest request;
    request.timerId = "42";
    request.backendId = "living-room";
    request.channelId = "C-61441-10006-50021";
    request.title = "Tagesschau";
    request.directory = "News";
    request.day = "2026-06-18";
    request.weekdays = "-------";
    request.start = 2015;
    request.stop = 2030;
    request.priority = 50;
    request.lifetime = 99;
    request.active = true;
    return request;
}

static void test_adapter_exposes_backend_id()
{
    MockHttpClient httpClient;

    RestfulApiVdrTimerActionExecutorAdapter adapter(
        "living-room",
        "/api",
        httpClient);

    assert(adapter.backendId() == "living-room");
}

static void test_adapter_exposes_executor()
{
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 201;
    response.body = "timer created";
    httpClient.setResponse(response);

    RestfulApiVdrTimerActionExecutorAdapter adapter(
        "living-room",
        "/api",
        httpClient);

    VdrTimerActionResult result =
        adapter.executor().execute(
            VdrTimerActionType::Create,
            makeRequest());

    assert(result.success == true);
    assert(result.backendId == "living-room");
    assert(result.timerId == "42");

    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "POST");
    assert(httpClient.lastRequest().url == "/api/timers");
}

static void test_adapter_can_be_registered()
{
    MockHttpClient httpClient;

    auto adapter =
        std::make_shared<RestfulApiVdrTimerActionExecutorAdapter>(
            "living-room",
            "/api",
            httpClient);

    VdrTimerActionExecutorAdapterRegistry registry;
    registry.registerAdapter(adapter);

    const VdrTimerActionExecutorAdapterLookupResult result =
        registry.findAdapter("living-room");

    assert(result.found == true);
    assert(result.adapter == adapter);
    assert(result.adapter->backendId() == "living-room");
}

int main()
{
    test_adapter_exposes_backend_id();
    test_adapter_exposes_executor();
    test_adapter_can_be_registered();

    return 0;
}

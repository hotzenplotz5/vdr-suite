#include "RestfulApiVdrTimerActionExecutor.h"
#include "MockHttpClient.h"

#include <cassert>
#include <string>

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

static void test_create_success_executes_post_request()
{
    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 201;
    response.body = "timer created";
    httpClient.setResponse(response);

    RestfulApiVdrTimerActionExecutor executor(
        "living-room",
        "/api",
        httpClient);

    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Create, request);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Create);
    assert(result.timerId == "42");
    assert(result.backendId == "living-room");
    assert(result.message == "RESTfulAPI timer action request executed");

    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "POST");
    assert(httpClient.lastRequest().url == "/api/timers");
    assert(httpClient.lastRequest().body.find("file=News~Tagesschau") != std::string::npos);
}

static void test_update_success_executes_put_request()
{
    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 200;
    response.body = "timer updated";
    httpClient.setResponse(response);

    RestfulApiVdrTimerActionExecutor executor(
        "living-room",
        "/api",
        httpClient);

    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Update, request);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Update);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "PUT");
    assert(httpClient.lastRequest().url == "/api/timers");
    assert(httpClient.lastRequest().body.find("timer_id=42") != std::string::npos);
}

static void test_delete_success_executes_delete_request()
{
    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 200;
    response.body = "Timer deleted.";
    httpClient.setResponse(response);

    RestfulApiVdrTimerActionExecutor executor(
        "living-room",
        "/api",
        httpClient);

    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Delete, request);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Delete);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "DELETE");
    assert(httpClient.lastRequest().url == "/api/timers/42");
    assert(httpClient.lastRequest().body.empty() == true);
}

static void test_http_error_preserves_status_and_body()
{
    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 404;
    response.body = "Timer id invalid!";
    httpClient.setResponse(response);

    RestfulApiVdrTimerActionExecutor executor(
        "living-room",
        "/api",
        httpClient);

    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Delete, request);

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Delete);
    assert(result.message == "RESTfulAPI timer action request failed");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "RESTfulAPI returned HTTP status 404: Timer id invalid!");
}

static void test_http_error_without_body_preserves_status()
{
    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 502;
    response.body = "";
    httpClient.setResponse(response);

    RestfulApiVdrTimerActionExecutor executor(
        "living-room",
        "/api",
        httpClient);

    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Create, request);

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Create);
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "RESTfulAPI returned HTTP status 502");
}

static void test_toggle_is_not_supported_yet()
{
    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 200;
    response.body = "should not be used";
    httpClient.setResponse(response);

    RestfulApiVdrTimerActionExecutor executor(
        "living-room",
        "/api",
        httpClient);

    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Toggle, request);

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Toggle);
    assert(result.backendId == "living-room");
    assert(result.message == "RESTfulAPI timer action type not supported");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "unsupported timer action type for RESTfulAPI executor");
    assert(httpClient.requestCount() == 0);
}

int main()
{
    test_create_success_executes_post_request();
    test_update_success_executes_put_request();
    test_delete_success_executes_delete_request();
    test_http_error_preserves_status_and_body();
    test_http_error_without_body_preserves_status();
    test_toggle_is_not_supported_yet();

    return 0;
}

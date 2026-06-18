#include "RestfulApiVdrTimerActionRequestBuilder.h"

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
    request.aux = "<epgsearch></epgsearch>";
    return request;
}

static void test_build_create_request()
{
    RestfulApiVdrTimerActionRequestBuilder builder;
    const VdrTimerOperationRequest request = makeRequest();

    const HttpRequest httpRequest =
        builder.buildCreateRequest("/api", request);

    assert(httpRequest.method == "POST");
    assert(httpRequest.url == "/api/timers");
    assert(httpRequest.headers.at("Accept") == "application/json");
    assert(httpRequest.headers.at("Content-Type") == "application/x-www-form-urlencoded");

    assert(httpRequest.body.find("flags=1") != std::string::npos);
    assert(httpRequest.body.find("channel=C-61441-10006-50021") != std::string::npos);
    assert(httpRequest.body.find("weekdays=-------") != std::string::npos);
    assert(httpRequest.body.find("day=2026-06-18") != std::string::npos);
    assert(httpRequest.body.find("start=2015") != std::string::npos);
    assert(httpRequest.body.find("stop=2030") != std::string::npos);
    assert(httpRequest.body.find("priority=50") != std::string::npos);
    assert(httpRequest.body.find("lifetime=99") != std::string::npos);
    assert(httpRequest.body.find("file=News~Tagesschau") != std::string::npos);
    assert(httpRequest.body.find("aux=<epgsearch></epgsearch>") != std::string::npos);
    assert(httpRequest.body.find("timer_id=") == std::string::npos);
}

static void test_build_update_request()
{
    RestfulApiVdrTimerActionRequestBuilder builder;
    const VdrTimerOperationRequest request = makeRequest();

    const HttpRequest httpRequest =
        builder.buildUpdateRequest("/api/", request);

    assert(httpRequest.method == "PUT");
    assert(httpRequest.url == "/api/timers");
    assert(httpRequest.body.find("timer_id=42") != std::string::npos);
    assert(httpRequest.body.find("file=News~Tagesschau") != std::string::npos);
}

static void test_build_delete_request()
{
    RestfulApiVdrTimerActionRequestBuilder builder;
    const VdrTimerOperationRequest request = makeRequest();

    const HttpRequest httpRequest =
        builder.buildDeleteRequest("", request);

    assert(httpRequest.method == "DELETE");
    assert(httpRequest.url == "/timers/42");
    assert(httpRequest.headers.at("Accept") == "application/json");
    assert(httpRequest.body.empty() == true);
}

static void test_inactive_request_sets_flags_zero()
{
    RestfulApiVdrTimerActionRequestBuilder builder;
    VdrTimerOperationRequest request = makeRequest();
    request.active = false;

    const HttpRequest httpRequest =
        builder.buildCreateRequest("", request);

    assert(httpRequest.body.find("flags=0") != std::string::npos);
}

static void test_request_without_directory_uses_title_as_file()
{
    RestfulApiVdrTimerActionRequestBuilder builder;
    VdrTimerOperationRequest request = makeRequest();
    request.directory.clear();

    const HttpRequest httpRequest =
        builder.buildCreateRequest("", request);

    assert(httpRequest.body.find("file=Tagesschau") != std::string::npos);
}

int main()
{
    test_build_create_request();
    test_build_update_request();
    test_build_delete_request();
    test_inactive_request_sets_flags_zero();
    test_request_without_directory_uses_title_as_file();

    return 0;
}

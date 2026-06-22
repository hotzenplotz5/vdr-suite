#include "RestfulApiSearchTimerCommandExecutor.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerDeleteRequest.h"
#include "SearchTimerUpdateRequest.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class TestHttpClient final : public IHttpClient
{
public:
    explicit TestHttpClient(
        HttpResponse response)
        : response_(response)
    {
    }

    HttpResponse execute(
        const HttpRequest& request) const override
    {
        requests.push_back(request);
        return response_;
    }

    mutable std::vector<HttpRequest> requests;

private:
    HttpResponse response_;
};

static SearchTimerCreateRequest makeCreateRequest()
{
    SearchTimerCreateRequest request;
    request.backendId = "home-vdr";
    request.name = "Tatort";
    request.query = "Tatort";
    request.active = true;
    request.directory = "Doku";
    request.priority = 50;
    request.lifetime = 99;
    request.marginStartMinutes = 5;
    request.marginStopMinutes = 10;
    request.useVps = true;
    request.useChannel = 2;
    request.channels = "ZDF";
    request.channelMin = "S19.2E-1-1011-11110";
    request.channelMax = "S19.2E-1-1011-11120";
    request.useTime = true;
    request.startTime = 2015;
    request.stopTime = 2230;
    request.useDuration = true;
    request.durationMinMinutes = 45;
    request.durationMaxMinutes = 120;
    request.useDayOfWeek = true;
    request.dayOfWeek = 62;
    request.avoidRepeats = true;
    request.allowedRepeats = 2;
    request.repeatsWithinDays = 14;
    request.compareTitle = true;
    request.compareSubtitle = true;
    request.compareSummary = true;
    request.compareCategories = true;
    request.compareTime = true;
    return request;
}

static SearchTimerUpdateRequest makeUpdateRequest()
{
    SearchTimerUpdateRequest request;
    request.backendId = "home-vdr";
    request.backendNativeId = "42";
    request.name = "Tatort updated";
    request.query = "Tatort updated";
    request.active = true;
    request.directory = "Serien";
    request.priority = 60;
    request.lifetime = 88;
    request.marginStartMinutes = 7;
    request.marginStopMinutes = 12;
    request.useVps = true;
    request.useChannel = 1;
    request.channels = "";
    request.channelMin = "S19.2E-1-1011-11110";
    request.channelMax = "S19.2E-1-1011-11120";
    request.useTime = true;
    request.startTime = 2100;
    request.stopTime = 2300;
    request.useDuration = true;
    request.durationMinMinutes = 30;
    request.durationMaxMinutes = 90;
    request.useDayOfWeek = true;
    request.dayOfWeek = 31;
    request.avoidRepeats = true;
    request.allowedRepeats = 3;
    request.repeatsWithinDays = 21;
    request.compareTitle = true;
    request.compareSubtitle = false;
    request.compareSummary = true;
    request.compareCategories = false;
    request.compareTime = true;
    return request;
}

static void test_create_posts_to_restfulapi_searchtimers()
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = "OK, Id:42";

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const SearchTimerCreateResult result =
        executor.create(makeCreateRequest());

    assert(result.success == true);
    assert(result.searchTimer.id().backendId() == "home-vdr");
    assert(result.searchTimer.id().nativeId() == "42");
    assert(result.searchTimer.query() == "Tatort");
    assert(httpClient.requests.size() == 1);
    assert(httpClient.requests.at(0).method == "POST");
    assert(httpClient.requests.at(0).url == "/searchtimers");
    assert(httpClient.requests.at(0).headers.at("Content-Type") == "application/json");
    assert(httpClient.requests.at(0).body.find("\"search\":\"Tatort\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"directory\":\"Doku\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"priority\":50") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"lifetime\":99") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"margin_start\":5") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"margin_stop\":10") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_vps\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_channel\":2") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"channels\":\"ZDF\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"channel_min\":\"S19.2E-1-1011-11110\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"channel_max\":\"S19.2E-1-1011-11120\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_time\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"start_time\":2015") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"stop_time\":2230") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_duration\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"duration_min\":45") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"duration_max\":120") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_dayofweek\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"dayofweek\":62") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"avoid_repeats\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"allowed_repeats\":2") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"repeats_within_days\":14") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_title\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_subtitle\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_summary\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_categories\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_time\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_as_searchtimer\":1") != std::string::npos);
}

static void test_create_fails_without_created_id()
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = "OK";

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const SearchTimerCreateResult result =
        executor.create(makeCreateRequest());

    assert(result.success == false);
    assert(result.message == "RESTfulAPI searchtimer create did not return an id");
    assert(result.errors.size() == 1);
}

static void test_update_puts_to_restfulapi_searchtimer_by_native_id()
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = "OK, Id:42";

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const SearchTimerUpdateResult result =
        executor.update(makeUpdateRequest());

    assert(result.success == true);
    assert(result.searchTimer.id().backendId() == "home-vdr");
    assert(result.searchTimer.id().nativeId() == "42");
    assert(result.searchTimer.name() == "Tatort updated");
    assert(result.searchTimer.query() == "Tatort updated");
    assert(httpClient.requests.size() == 1);
    assert(httpClient.requests.at(0).method == "PUT");
    assert(httpClient.requests.at(0).url == "/searchtimers/42");
    assert(httpClient.requests.at(0).headers.at("Content-Type") == "application/json");
    assert(httpClient.requests.at(0).body.find("\"search\":\"Tatort updated\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"directory\":\"Serien\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"priority\":60") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"lifetime\":88") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"margin_start\":7") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"margin_stop\":12") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_vps\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_channel\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"channel_min\":\"S19.2E-1-1011-11110\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"channel_max\":\"S19.2E-1-1011-11120\"") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_time\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"start_time\":2100") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"stop_time\":2300") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_duration\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"duration_min\":30") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"duration_max\":90") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_dayofweek\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"dayofweek\":31") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"avoid_repeats\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"allowed_repeats\":3") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"repeats_within_days\":21") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_title\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_subtitle\":0") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_summary\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_categories\":0") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"compare_time\":1") != std::string::npos);
    assert(httpClient.requests.at(0).body.find("\"use_as_searchtimer\":1") != std::string::npos);
}

static void test_update_fails_without_returned_id()
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = "OK";

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const SearchTimerUpdateResult result =
        executor.update(makeUpdateRequest());

    assert(result.success == false);
    assert(result.message == "RESTfulAPI searchtimer update did not return an id");
    assert(result.errors.size() == 1);
}

static void test_remove_deletes_restfulapi_searchtimer_by_native_id()
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = "Searchtimer deleted.";

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    SearchTimerDeleteRequest request;
    request.backendId = "home-vdr";
    request.backendNativeId = "42";

    const SearchTimerDeleteResult result =
        executor.remove(request);

    assert(result.success == true);
    assert(result.backendId == "home-vdr");
    assert(result.backendNativeId == "42");
    assert(httpClient.requests.size() == 1);
    assert(httpClient.requests.at(0).method == "DELETE");
    assert(httpClient.requests.at(0).url == "/searchtimers/42");
}

int main()
{
    test_create_posts_to_restfulapi_searchtimers();
    test_create_fails_without_created_id();
    test_update_puts_to_restfulapi_searchtimer_by_native_id();
    test_update_fails_without_returned_id();
    test_remove_deletes_restfulapi_searchtimer_by_native_id();

    std::cout
        << "test_restfulapi_search_timer_command_executor passed"
        << std::endl;

    return 0;
}

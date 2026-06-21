#include "SearchTimerController.h"

#include "SearchTimerResult.h"
#include "ISearchTimerCommandExecutor.h"
#include "ISearchTimerDataSource.h"
#include "SearchTimerCreateRequestParser.h"
#include "SearchTimerCreateResultJsonSerializer.h"
#include "SearchTimerCreateService.h"
#include "SearchTimerUpdateRequestParser.h"
#include "SearchTimerUpdateResultJsonSerializer.h"
#include "SearchTimerUpdateService.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerService.h"
#include "VdrEvent.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class TestSearchTimerDataSource final : public ISearchTimerDataSource
{
public:
    TestSearchTimerDataSource()
    {
        timers_.push_back(SearchTimer::create(
            SearchTimerId::fromBackendNativeId("default", "1"),
            "Controller SearchTimer Terra X",
            "Terra X",
            SearchTimerState::Active));
    }

    SearchTimerResult list(
        const SearchTimerQuery& query) const override
    {
        SearchTimerService service;
        return service.list(timers_, query);
    }

private:
    std::vector<SearchTimer> timers_;
};

class TestSearchTimerCommandExecutor final : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++callCount_;

        return SearchTimerCreateResult::ok(
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    request.backendId,
                    "created-searchtimer-1"),
                request.name,
                request.query,
                request.active ? SearchTimerState::Active : SearchTimerState::Inactive),
            "searchtimer created");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        ++updateCallCount_;

        return SearchTimerUpdateResult::ok(
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    request.backendId,
                    request.backendNativeId),
                request.name,
                request.query,
                request.active ? SearchTimerState::Active : SearchTimerState::Inactive),
            "searchtimer updated");
    }

    int callCount() const
    {
        return callCount_;
    }

    int updateCallCount() const
    {
        return updateCallCount_;
    }

private:
    int callCount_ = 0;
    int updateCallCount_ = 0;
};

int main()
{
    SearchTimerService searchTimerService;
    SearchTimerResultJsonSerializer serializer;
    TestSearchTimerDataSource dataSource;
    SearchTimerCreateService createService;
    SearchTimerCreateResultJsonSerializer createJsonSerializer;
    SearchTimerCreateRequestParser createRequestParser;
    SearchTimerUpdateService updateService;
    SearchTimerUpdateResultJsonSerializer updateJsonSerializer;
    SearchTimerUpdateRequestParser updateRequestParser;
    SearchTimerController controller(
        searchTimerService,
        serializer,
        dataSource,
        createService,
        createJsonSerializer,
        createRequestParser,
        &updateService,
        &updateJsonSerializer,
        &updateRequestParser);

    std::vector<SearchTimer> timers;
    timers.push_back(SearchTimer::create(
        SearchTimerId::fromBackendNativeId("livingroom", "1"),
        "Terra X",
        "Terra X",
        SearchTimerState::Active));

    SearchTimerResult result = SearchTimerResult::from(
        timers,
        1,
        25,
        0);

    ApiResponse response = controller.getSearchTimers(result);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"searchtimers\"") != std::string::npos);
    assert(response.body.find("\"backendId\":\"livingroom\"") != std::string::npos);
    assert(response.body.find("\"backendNativeId\":\"1\"") != std::string::npos);
    assert(response.body.find("\"name\":\"Terra X\"") != std::string::npos);
    assert(response.body.find("\"state\":\"active\"") != std::string::npos);

    VdrEvent event;
    event.id = "event-1";
    event.channelId = "channel-1";
    event.title = "Terra X";
    event.startTime = "1000";
    event.endTime = "1100";
    event.durationSeconds = 3600;

    ApiResponse previewResponse =
        controller.previewSearchTimer(
            timers[0],
            {event},
            10,
            0);

    assert(previewResponse.statusCode == 200);
    assert(previewResponse.contentType == "application/json");
    assert(previewResponse.body.find("\"searchTimer\":{") != std::string::npos);
    assert(previewResponse.body.find("\"preview\":{") != std::string::npos);
    assert(previewResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(previewResponse.body.find("\"eventId\":\"event-1\"") != std::string::npos);

    TestSearchTimerCommandExecutor executor;
    ApiResponse createResponse =
        controller.createSearchTimer(
            "{"
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Terra X Suche\","
            "\"query\":\"Terra X\","
            "\"active\":true"
            "}",
            executor);

    assert(createResponse.statusCode == 200);
    assert(createResponse.contentType == "application/json");
    assert(createResponse.body.find("\"success\":true") != std::string::npos);
    assert(createResponse.body.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(createResponse.body.find("\"backendNativeId\":\"created-searchtimer-1\"") != std::string::npos);
    assert(createResponse.body.find("\"name\":\"Terra X Suche\"") != std::string::npos);
    assert(createResponse.body.find("\"query\":\"Terra X\"") != std::string::npos);
    assert(executor.callCount() == 1);

    ApiResponse updateResponse =
        controller.updateSearchTimer(
            "{"
            "\"backendId\":\"home-vdr\","
            "\"backendNativeId\":\"searchtimer-42\","
            "\"name\":\"Terra X Suche aktualisiert\","
            "\"query\":\"Terra X aktualisiert\","
            "\"active\":false"
            "}",
            executor);

    assert(updateResponse.statusCode == 200);
    assert(updateResponse.contentType == "application/json");
    assert(updateResponse.body.find("\"success\":true") != std::string::npos);
    assert(updateResponse.body.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(updateResponse.body.find("\"backendNativeId\":\"searchtimer-42\"") != std::string::npos);
    assert(updateResponse.body.find("\"name\":\"Terra X Suche aktualisiert\"") != std::string::npos);
    assert(updateResponse.body.find("\"query\":\"Terra X aktualisiert\"") != std::string::npos);
    assert(updateResponse.body.find("\"state\":\"inactive\"") != std::string::npos);
    assert(executor.updateCallCount() == 1);

    std::cout << "test_search_timer_controller passed" << std::endl;
    return 0;
}
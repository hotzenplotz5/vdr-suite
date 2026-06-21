#include "SearchTimerResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    SearchTimerResultJsonSerializer serializer;

    SearchTimerResult emptyResult = SearchTimerResult::empty(25, 10);
    const std::string emptyJson = serializer.serialize(emptyResult);
    assert(emptyJson.find("\"totalCount\":0") != std::string::npos);
    assert(emptyJson.find("\"returnedCount\":0") != std::string::npos);
    assert(emptyJson.find("\"limit\":25") != std::string::npos);
    assert(emptyJson.find("\"offset\":10") != std::string::npos);
    assert(emptyJson.find("\"searchtimers\":[]") != std::string::npos);

    std::vector<SearchTimer> timers;
    SearchTimer terraX = SearchTimer::create(
        SearchTimerId::fromBackendNativeId("livingroom", "1"),
        "Terra X",
        "Terra X",
        SearchTimerState::Active);
    terraX.recordingOptions().setDirectory("Doku");
    terraX.recordingOptions().setPriority(50);
    terraX.recordingOptions().setLifetime(99);
    terraX.scheduleOptions().setMarginStartMinutes(5);
    terraX.scheduleOptions().setMarginStopMinutes(10);
    terraX.scheduleOptions().setUseVps(true);
    terraX.filterOptions().setUseChannel(true);
    terraX.filterOptions().setUseDayOfWeek(true);
    terraX.filterOptions().setUseDuration(true);
    terraX.filterOptions().setDurationMinMinutes(30);
    terraX.filterOptions().setDurationMaxMinutes(120);
    terraX.comparisonOptions().setCompareTitle(true);
    terraX.comparisonOptions().setCompareSubtitle(true);
    terraX.comparisonOptions().setCompareSummary(true);
    terraX.comparisonOptions().setCompareCategories(true);
    terraX.comparisonOptions().setCompareTime(true);
    terraX.repeatOptions().setAvoidRepeats(true);
    terraX.repeatOptions().setAllowedRepeats(3);
    terraX.repeatOptions().setRepeatsWithinDays(14);
    terraX.channelOptions().setChannels("1,2,3");
    terraX.channelOptions().setChannelMin(1);
    terraX.channelOptions().setChannelMax(99);
    terraX.seriesOptions().setUseSeriesRecording(true);
    terraX.seriesOptions().setKeepRecordings(10);
    terraX.seriesOptions().setDeleteMode(2);
    terraX.seriesOptions().setSearchTimerAction(1);
    timers.push_back(terraX);
    timers.push_back(SearchTimer::create(
        SearchTimerId::fromBackendNativeId("bedroom", "2"),
        "Bob \"Marley\"",
        "Bob\\Marley",
        SearchTimerState::Inactive));

    SearchTimerResult result = SearchTimerResult::from(
        timers,
        7,
        2,
        4);

    const std::string json = serializer.serialize(result);

    assert(json.find("\"totalCount\":7") != std::string::npos);
    assert(json.find("\"returnedCount\":2") != std::string::npos);
    assert(json.find("\"limit\":2") != std::string::npos);
    assert(json.find("\"offset\":4") != std::string::npos);
    assert(json.find("\"backendId\":\"livingroom\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"1\"") != std::string::npos);
    assert(json.find("\"name\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"query\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"state\":\"active\"") != std::string::npos);
    assert(json.find("\"recordingOptions\":{") != std::string::npos);
    assert(json.find("\"directory\":\"Doku\"") != std::string::npos);
    assert(json.find("\"priority\":50") != std::string::npos);
    assert(json.find("\"lifetime\":99") != std::string::npos);
    assert(json.find("\"scheduleOptions\":{") != std::string::npos);
    assert(json.find("\"marginStartMinutes\":5") != std::string::npos);
    assert(json.find("\"marginStopMinutes\":10") != std::string::npos);
    assert(json.find("\"useVps\":true") != std::string::npos);
    assert(json.find("\"filterOptions\":{") != std::string::npos);
    assert(json.find("\"useChannel\":true") != std::string::npos);
    assert(json.find("\"useDayOfWeek\":true") != std::string::npos);
    assert(json.find("\"useDuration\":true") != std::string::npos);
    assert(json.find("\"durationMinMinutes\":30") != std::string::npos);
    assert(json.find("\"durationMaxMinutes\":120") != std::string::npos);
    assert(json.find("\"comparisonOptions\":{") != std::string::npos);
    assert(json.find("\"compareTitle\":true") != std::string::npos);
    assert(json.find("\"compareSubtitle\":true") != std::string::npos);
    assert(json.find("\"compareSummary\":true") != std::string::npos);
    assert(json.find("\"compareCategories\":true") != std::string::npos);
    assert(json.find("\"compareTime\":true") != std::string::npos);
    assert(json.find("\"repeatOptions\":{") != std::string::npos);
    assert(json.find("\"avoidRepeats\":true") != std::string::npos);
    assert(json.find("\"allowedRepeats\":3") != std::string::npos);
    assert(json.find("\"repeatsWithinDays\":14") != std::string::npos);
    assert(json.find("\"channelOptions\":{") != std::string::npos);
    assert(json.find("\"channels\":\"1,2,3\"") != std::string::npos);
    assert(json.find("\"channelMin\":1") != std::string::npos);
    assert(json.find("\"channelMax\":99") != std::string::npos);
    assert(json.find("\"seriesOptions\":{") != std::string::npos);
    assert(json.find("\"useSeriesRecording\":true") != std::string::npos);
    assert(json.find("\"keepRecordings\":10") != std::string::npos);
    assert(json.find("\"deleteMode\":2") != std::string::npos);
    assert(json.find("\"searchTimerAction\":1") != std::string::npos);
    assert(json.find("\"backendId\":\"bedroom\"") != std::string::npos);
    assert(json.find("\"name\":\"Bob \\\"Marley\\\"\"") != std::string::npos);
    assert(json.find("\"query\":\"Bob\\\\Marley\"") != std::string::npos);
    assert(json.find("\"state\":\"inactive\"") != std::string::npos);

    std::cout << "test_search_timer_result_json_serializer passed" << std::endl;
    return 0;
}
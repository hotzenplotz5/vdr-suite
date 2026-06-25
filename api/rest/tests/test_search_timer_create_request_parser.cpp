#include "SearchTimerCreateRequestParser.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerCreateRequestParser parser;

    const SearchTimerCreateRequest request =
        parser.parse(
            "{"
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Terra X Suche\","
            "\"query\":\"Terra X\","
            "\"active\":false,"
            "\"directory\":\"Doku\","
            "\"priority\":50,"
            "\"lifetime\":99,"
            "\"marginStartMinutes\":5,"
            "\"marginStopMinutes\":10,"
            "\"useVps\":true,"
            "\"useChannel\":2,"
            "\"channels\":\"ZDF\","
            "\"channelMin\":\"S19.2E-1-1011-11110\","
            "\"channelMax\":\"S19.2E-1-1011-11120\","
            "\"useTime\":true,"
            "\"startTime\":2015,"
            "\"stopTime\":2230,"
            "\"useDuration\":true,"
            "\"durationMinMinutes\":45,"
            "\"durationMaxMinutes\":120,"
            "\"useDayOfWeek\":true,"
            "\"dayOfWeek\":62,"
            "\"avoidRepeats\":true,"
            "\"allowedRepeats\":2,"
            "\"repeatsWithinDays\":14,"
            "\"compareTitle\":true,"
            "\"compareSubtitle\":true,"
            "\"compareSummary\":true,"
            "\"compareCategories\":true,"
            "\"compareTime\":true,"
            "\"useSeriesRecording\":true,"
            "\"keepRecordings\":5,"
            "\"deleteMode\":1,"
            "\"searchTimerAction\":2,"
            "\"blacklistMode\":1,"
            "\"blacklistIds\":\"3,7\","
            "\"mode\":\"fuzzy\","
            "\"matchCase\":true,"
            "\"tolerance\":2,"
            "\"summaryMatch\":1,"
            "\"useExtendedEpgInfo\":true,"
            "\"extendedEpgInfo\":\"category=movie\","
            "\"ignoreMissingEpgCategories\":true,"
            "\"contentDescriptors\":\"0x10,0x11\","
            "\"useInFavorites\":true,"
            "\"activeFrom\":\"2026-06-01\","
            "\"activeUntil\":\"2026-12-31\","
            "\"pauseOnRecordings\":true,"
            "\"switchMinutesBefore\":3,"
            "\"unmuteSoundOnSwitch\":true,"
            "\"deleteRecordingsAfterDays\":30,"
            "\"deleteAfterCountRecordings\":5,"
            "\"deleteAfterDaysOfFirstRecording\":90"
            "}");

    assert(request.backendId == "home-vdr");
    assert(request.name == "Terra X Suche");
    assert(request.query == "Terra X");
    assert(request.active == false);
    assert(request.directory == "Doku");
    assert(request.priority == 50);
    assert(request.lifetime == 99);
    assert(request.marginStartMinutes == 5);
    assert(request.marginStopMinutes == 10);
    assert(request.useVps == true);
    assert(request.useChannel == 2);
    assert(request.channels == "ZDF");
    assert(request.channelMin == "S19.2E-1-1011-11110");
    assert(request.channelMax == "S19.2E-1-1011-11120");
    assert(request.useTime == true);
    assert(request.startTime == 2015);
    assert(request.stopTime == 2230);
    assert(request.useDuration == true);
    assert(request.durationMinMinutes == 45);
    assert(request.durationMaxMinutes == 120);
    assert(request.useDayOfWeek == true);
    assert(request.dayOfWeek == 62);
    assert(request.avoidRepeats == true);
    assert(request.allowedRepeats == 2);
    assert(request.repeatsWithinDays == 14);
    assert(request.compareTitle == true);
    assert(request.compareSubtitle == true);
    assert(request.compareSummary == true);
    assert(request.compareCategories == true);
    assert(request.compareTime == true);
    assert(request.useSeriesRecording == true);
    assert(request.keepRecordings == 5);
    assert(request.deleteMode == 1);
    assert(request.searchTimerAction == 2);
    assert(request.blacklistMode == 1);
    assert(request.blacklistIds == "3,7");
    assert(request.matchMode == 5);
    assert(request.matchCase == true);
    assert(request.matchTolerance == 2);
    assert(request.summaryMatch == 1);
    assert(request.useExtendedEpgInfo == true);
    assert(request.extendedEpgInfo == "category=movie");
    assert(request.ignoreMissingEpgCategories == true);
    assert(request.contentDescriptors == "0x10,0x11");
    assert(request.useInFavorites == true);
    assert(request.activeFrom == "2026-06-01");
    assert(request.activeUntil == "2026-12-31");
    assert(request.pauseOnRecordings == true);
    assert(request.switchMinutesBefore == 3);
    assert(request.unmuteSoundOnSwitch == true);
    assert(request.deleteRecordingsAfterDays == 30);
    assert(request.deleteAfterCountRecordings == 5);
    assert(request.deleteAfterDaysOfFirstRecording == 90);

    const SearchTimerCreateRequest defaultBackendRequest =
        parser.parse(
            "{"
            "\"name\":\"Default Suche\","
            "\"query\":\"Default\","
            "\"active\":true"
            "}");

    assert(defaultBackendRequest.backendId == "default");
    assert(defaultBackendRequest.name == "Default Suche");
    assert(defaultBackendRequest.query == "Default");
    assert(defaultBackendRequest.active == true);
    assert(defaultBackendRequest.directory.empty());
    assert(defaultBackendRequest.priority == 0);
    assert(defaultBackendRequest.lifetime == 0);
    assert(defaultBackendRequest.marginStartMinutes == 0);
    assert(defaultBackendRequest.marginStopMinutes == 0);
    assert(defaultBackendRequest.useVps == false);
    assert(defaultBackendRequest.useChannel == 0);
    assert(defaultBackendRequest.channels.empty());
    assert(defaultBackendRequest.channelMin.empty());
    assert(defaultBackendRequest.channelMax.empty());
    assert(defaultBackendRequest.useTime == false);
    assert(defaultBackendRequest.startTime == 0);
    assert(defaultBackendRequest.stopTime == 0);
    assert(defaultBackendRequest.useDuration == false);
    assert(defaultBackendRequest.durationMinMinutes == 0);
    assert(defaultBackendRequest.durationMaxMinutes == 0);
    assert(defaultBackendRequest.useDayOfWeek == false);
    assert(defaultBackendRequest.dayOfWeek == 0);
    assert(defaultBackendRequest.avoidRepeats == false);
    assert(defaultBackendRequest.allowedRepeats == 0);
    assert(defaultBackendRequest.repeatsWithinDays == 0);
    assert(defaultBackendRequest.compareTitle == false);
    assert(defaultBackendRequest.compareSubtitle == false);
    assert(defaultBackendRequest.compareSummary == false);
    assert(defaultBackendRequest.compareCategories == false);
    assert(defaultBackendRequest.compareTime == false);
    assert(defaultBackendRequest.useSeriesRecording == false);
    assert(defaultBackendRequest.keepRecordings == 0);
    assert(defaultBackendRequest.deleteMode == 0);
    assert(defaultBackendRequest.searchTimerAction == 0);
    assert(defaultBackendRequest.blacklistMode == 0);
    assert(defaultBackendRequest.blacklistIds.empty());
    assert(defaultBackendRequest.matchMode == 0);
    assert(defaultBackendRequest.matchCase == false);
    assert(defaultBackendRequest.matchTolerance == 0);
    assert(defaultBackendRequest.summaryMatch == 0);
    assert(defaultBackendRequest.useExtendedEpgInfo == false);
    assert(defaultBackendRequest.extendedEpgInfo.empty());
    assert(defaultBackendRequest.ignoreMissingEpgCategories == false);
    assert(defaultBackendRequest.contentDescriptors.empty());
    assert(defaultBackendRequest.useInFavorites == false);
    assert(defaultBackendRequest.activeFrom.empty());
    assert(defaultBackendRequest.activeUntil.empty());
    assert(defaultBackendRequest.pauseOnRecordings == false);
    assert(defaultBackendRequest.switchMinutesBefore == 0);
    assert(defaultBackendRequest.unmuteSoundOnSwitch == false);
    assert(defaultBackendRequest.deleteRecordingsAfterDays == 0);
    assert(defaultBackendRequest.deleteAfterCountRecordings == 0);
    assert(defaultBackendRequest.deleteAfterDaysOfFirstRecording == 0);


    const SearchTimerCreateRequest titleOnlyRequest =
        parser.parse(
            "{"
            "\"backendId\":\"default\","
            "\"name\":\"Amerika\","
            "\"query\":\"Amerika\","
            "\"active\":true,"
            "\"mode\":\"phrase\","
            "\"compareTitle\":true,"
            "\"compareSubtitle\":false,"
            "\"compareSummary\":false,"
            "\"compareCategories\":false"
            "}");

    assert(titleOnlyRequest.backendId == "default");
    assert(titleOnlyRequest.name == "Amerika");
    assert(titleOnlyRequest.query == "Amerika");
    assert(titleOnlyRequest.active == true);
    assert(titleOnlyRequest.matchMode == 0);
    assert(titleOnlyRequest.compareTitle == true);
    assert(titleOnlyRequest.compareSubtitle == false);
    assert(titleOnlyRequest.compareSummary == false);
    assert(titleOnlyRequest.compareCategories == false);

    const SearchTimerCreateRequest subtitleSummaryRequest =
        parser.parse(
            "{"
            "\"backendId\":\"default\","
            "\"name\":\"Amerika Zusatzfelder\","
            "\"query\":\"Amerika\","
            "\"active\":true,"
            "\"mode\":\"phrase\","
            "\"compareTitle\":false,"
            "\"compareSubtitle\":true,"
            "\"compareSummary\":true,"
            "\"compareCategories\":false"
            "}");

    assert(subtitleSummaryRequest.backendId == "default");
    assert(subtitleSummaryRequest.name == "Amerika Zusatzfelder");
    assert(subtitleSummaryRequest.query == "Amerika");
    assert(subtitleSummaryRequest.active == true);
    assert(subtitleSummaryRequest.matchMode == 0);
    assert(subtitleSummaryRequest.compareTitle == false);
    assert(subtitleSummaryRequest.compareSubtitle == true);
    assert(subtitleSummaryRequest.compareSummary == true);
    assert(subtitleSummaryRequest.compareCategories == false);

    std::cout
        << "test_search_timer_create_request_parser passed"
        << std::endl;

    return 0;
}

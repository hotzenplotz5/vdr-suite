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
            "\"compareTime\":true"
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

    std::cout
        << "test_search_timer_create_request_parser passed"
        << std::endl;

    return 0;
}

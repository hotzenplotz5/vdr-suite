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
            "\"useVps\":true"
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

    std::cout
        << "test_search_timer_create_request_parser passed"
        << std::endl;

    return 0;
}

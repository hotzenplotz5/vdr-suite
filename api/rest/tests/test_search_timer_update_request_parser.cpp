#include "SearchTimerUpdateRequestParser.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerUpdateRequestParser parser;

    const SearchTimerUpdateRequest request =
        parser.parse(
            "{"
            "\"backendId\":\"home-vdr\","
            "\"backendNativeId\":\"searchtimer-42\","
            "\"name\":\"Terra X Suche\","
            "\"query\":\"Terra X\","
            "\"active\":false"
            "}");

    assert(request.backendId == "home-vdr");
    assert(request.backendNativeId == "searchtimer-42");
    assert(request.name == "Terra X Suche");
    assert(request.query == "Terra X");
    assert(request.active == false);

    const SearchTimerUpdateRequest defaultBackendRequest =
        parser.parse(
            "{"
            "\"backendNativeId\":\"searchtimer-99\","
            "\"name\":\"Default Suche\","
            "\"query\":\"Default\","
            "\"active\":true"
            "}");

    assert(defaultBackendRequest.backendId == "default");
    assert(defaultBackendRequest.backendNativeId == "searchtimer-99");
    assert(defaultBackendRequest.name == "Default Suche");
    assert(defaultBackendRequest.query == "Default");
    assert(defaultBackendRequest.active == true);

    std::cout
        << "test_search_timer_update_request_parser passed"
        << std::endl;

    return 0;
}

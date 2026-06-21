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
            "\"active\":false"
            "}");

    assert(request.backendId == "home-vdr");
    assert(request.name == "Terra X Suche");
    assert(request.query == "Terra X");
    assert(request.active == false);

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

    std::cout
        << "test_search_timer_create_request_parser passed"
        << std::endl;

    return 0;
}

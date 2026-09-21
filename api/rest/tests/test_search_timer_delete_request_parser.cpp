#include "SearchTimerDeleteRequestParser.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerDeleteRequestParser parser;

    const SearchTimerDeleteRequest request =
        parser.parse(
            "{"
            "\"backendId\":\"home-vdr\","
            "\"backendNativeId\":\"searchtimer-42\""
            "}");

    assert(request.backendId == "home-vdr");
    assert(request.backendNativeId == "searchtimer-42");

    const SearchTimerDeleteRequest defaultBackendRequest =
        parser.parse(
            "{"
            "\"backendNativeId\":\"searchtimer-99\""
            "}");

    assert(defaultBackendRequest.backendId == "default");
    assert(defaultBackendRequest.backendNativeId == "searchtimer-99");

    std::cout
        << "test_search_timer_delete_request_parser passed"
        << std::endl;

    return 0;
}

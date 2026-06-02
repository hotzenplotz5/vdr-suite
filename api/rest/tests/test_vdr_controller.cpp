#include "MockVdrAdapter.h"
#include "VdrController.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrService.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    MockVdrAdapter adapter;

    VdrService vdrService(adapter);

    VdrOverviewService overviewService(
        vdrService);

    VdrOverviewJsonSerializer jsonSerializer;

    VdrController controller(
        overviewService,
        jsonSerializer);

    ApiResponse response =
        controller.getOverview();

    assert(response.statusCode == 200);

    assert(response.contentType
           == "application/json");

    assert(response.body.find("\"status\"")
           != std::string::npos);

    assert(response.body.find("\"channels\"")
           != std::string::npos);

    assert(response.body.find("\"events\"")
           != std::string::npos);

    assert(response.body.find("\"timers\"")
           != std::string::npos);

    assert(response.body.find("\"recordings\"")
           != std::string::npos);

    assert(response.body.find("\"totalChannels\":3")
           != std::string::npos);

    assert(response.body.find("\"totalEvents\":2")
           != std::string::npos);

    assert(response.body.find("\"totalRecordings\":2")
           != std::string::npos);

    std::cout
        << response.body
        << std::endl;

    std::cout
        << "test_vdr_controller passed"
        << std::endl;

    return 0;
}

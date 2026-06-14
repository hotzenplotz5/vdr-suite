#include "CapabilityController.h"

#include "CapabilityReport.h"
#include "CapabilityReportJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    std::vector<CapabilityState> states;
    states.push_back(CapabilityState::available("recordings.read"));
    states.push_back(
        CapabilityState::unsupported(
            "timers.create",
            "capability unsupported by backend"));

    CapabilityReport report(
        "living-room",
        states);

    CapabilityReportJsonSerializer serializer;
    CapabilityController controller(
        report,
        serializer);

    ApiResponse response =
        controller.getCapabilities();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(response.body.find("\"capability\":\"recordings.read\"") != std::string::npos);
    assert(response.body.find("\"availability\":\"available\"") != std::string::npos);
    assert(response.body.find("\"capability\":\"timers.create\"") != std::string::npos);
    assert(response.body.find("\"availability\":\"unsupported\"") != std::string::npos);

    std::cout
        << "test_capability_controller passed"
        << std::endl;

    return 0;
}

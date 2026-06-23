#include "CapabilityController.h"

#include "CapabilityReportJsonSerializer.h"
#include "CapabilityReportService.h"
#include "CapabilityReportBuilder.h"
#include "CapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrCapabilitySet capabilities =
        VdrCapabilitySet::snapshotReadOnly();

    CapabilityResolver resolver(capabilities);
    CapabilityReportBuilder builder;
    CapabilityReportService service(
        "living-room",
        resolver,
        builder);

    CapabilityReportJsonSerializer serializer;
    CapabilityController controller(
        service,
        serializer);

    ApiResponse response =
        controller.getCapabilities();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(response.body.find("\"capability\":\"recordings.read\"") != std::string::npos);
    assert(response.body.find("\"availability\":\"available\"") != std::string::npos);
    assert(response.body.find("\"availableNow\":true") != std::string::npos);
    assert(response.body.find("\"capability\":\"epg.search.fuzzy.fallback\"") != std::string::npos);
    assert(response.body.find("\"capability\":\"epg.search.fuzzy.native\"") != std::string::npos);

    std::cout
        << "test_capability_controller passed"
        << std::endl;

    return 0;
}

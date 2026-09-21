#include "SearchTimerDiscoveryController.h"

#include "ISearchTimerDiscoveryProvider.h"
#include "SearchTimerDiscoveryJsonSerializer.h"
#include "SearchTimerDiscoveryService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{
SearchTimerDiscoveryCatalog makeCatalog(
    const std::string& backendId)
{
    SearchTimerDiscoveryCatalog catalog;
    catalog.setBackendId(backendId);

    catalog.addExtendedEpgInfo(
        SearchTimerDiscoveryExtendedEpgInfo::create(
            7,
            "Category",
            std::vector<std::string>{"Movie", "Series"},
            "7|Category|Movie,Series"));

    catalog.addChannelGroup(
        SearchTimerDiscoveryChannelGroup::create("HD Channels"));

    catalog.addBlacklist(
        SearchTimerDiscoveryBlacklist::create(
            4,
            "Teleshopping"));

    catalog.addRecordingDirectory(
        SearchTimerDiscoveryRecordingDirectory::create("Doku/Natur"));

    return catalog;
}

void assertDiscoveryBody(
    const ApiResponse& response,
    const std::string& backendId)
{
    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"" + backendId + "\"") != std::string::npos);
    assert(response.body.find("\"counts\":{") != std::string::npos);
    assert(response.body.find("\"extendedEpgInfo\":1") != std::string::npos);
    assert(response.body.find("\"channelGroups\":1") != std::string::npos);
    assert(response.body.find("\"blacklists\":1") != std::string::npos);
    assert(response.body.find("\"recordingDirectories\":1") != std::string::npos);
    assert(response.body.find("\"name\":\"Category\"") != std::string::npos);
    assert(response.body.find("\"values\":[\"Movie\",\"Series\"]") != std::string::npos);
    assert(response.body.find("\"channelGroups\":[{\"name\":\"HD Channels\"}]") != std::string::npos);
    assert(response.body.find("\"blacklists\":[{\"id\":4,\"search\":\"Teleshopping\"}]") != std::string::npos);
    assert(response.body.find("\"recordingDirectories\":[{\"path\":\"Doku/Natur\"}]") != std::string::npos);
}

class StaticDiscoveryProvider final
    : public ISearchTimerDiscoveryProvider
{
public:
    SearchTimerDiscoveryCatalog discover(
        const std::string& backendId) const override
    {
        return makeCatalog(backendId);
    }
};
}

int main()
{
    SearchTimerDiscoveryJsonSerializer serializer;

    SearchTimerDiscoveryCatalog catalog =
        makeCatalog("livingroom");

    SearchTimerDiscoveryController catalogController(serializer);

    ApiResponse catalogResponse =
        catalogController.getDiscovery(catalog);

    assertDiscoveryBody(
        catalogResponse,
        "livingroom");

    ApiResponse missingServiceResponse =
        catalogController.getDiscovery("livingroom");

    assert(missingServiceResponse.statusCode == 503);
    assert(missingServiceResponse.contentType == "application/json");
    assert(missingServiceResponse.body.find("not configured") != std::string::npos);

    StaticDiscoveryProvider provider;
    SearchTimerDiscoveryService service(provider);

    SearchTimerDiscoveryController serviceController(
        service,
        serializer);

    ApiResponse serviceResponse =
        serviceController.getDiscovery("bedroom");

    assertDiscoveryBody(
        serviceResponse,
        "bedroom");

    std::cout
        << "test_search_timer_discovery_controller passed"
        << std::endl;

    return 0;
}

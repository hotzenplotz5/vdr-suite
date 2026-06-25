#include "SearchTimerDiscoveryController.h"

#include "SearchTimerDiscoveryJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    SearchTimerDiscoveryCatalog catalog;
    catalog.setBackendId("livingroom");

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

    SearchTimerDiscoveryJsonSerializer serializer;
    SearchTimerDiscoveryController controller(serializer);

    ApiResponse response =
        controller.getDiscovery(catalog);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"livingroom\"") != std::string::npos);
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

    std::cout
        << "test_search_timer_discovery_controller passed"
        << std::endl;

    return 0;
}

#include "SearchTimerDiscoveryJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    SearchTimerDiscoveryCatalog emptyCatalog;
    SearchTimerDiscoveryJsonSerializer serializer;

    const std::string emptyJson =
        serializer.serialize(emptyCatalog);

    assert(emptyJson.find("\"backendId\":\"\"") != std::string::npos);
    assert(emptyJson.find("\"extendedEpgInfo\":0") != std::string::npos);
    assert(emptyJson.find("\"channelGroups\":0") != std::string::npos);
    assert(emptyJson.find("\"blacklists\":0") != std::string::npos);
    assert(emptyJson.find("\"recordingDirectories\":0") != std::string::npos);
    assert(emptyJson.find("\"extendedEpgInfo\":[]") != std::string::npos);
    assert(emptyJson.find("\"channelGroups\":[]") != std::string::npos);
    assert(emptyJson.find("\"blacklists\":[]") != std::string::npos);
    assert(emptyJson.find("\"recordingDirectories\":[]") != std::string::npos);

    SearchTimerDiscoveryCatalog catalog;
    catalog.setBackendId("livingroom");

    catalog.addExtendedEpgInfo(
        SearchTimerDiscoveryExtendedEpgInfo::create(
            7,
            "Category \"HD\"",
            std::vector<std::string>{"Movie", "Series"},
            "7|Category|Movie,Series"));

    catalog.addChannelGroup(
        SearchTimerDiscoveryChannelGroup::create("HD Channels"));

    catalog.addBlacklist(
        SearchTimerDiscoveryBlacklist::create(
            4,
            "Teleshopping"));

    catalog.addRecordingDirectory(
        SearchTimerDiscoveryRecordingDirectory::create(
            "Doku\\Natur"));

    const std::string json =
        serializer.serialize(catalog);

    assert(json.find("\"backendId\":\"livingroom\"") != std::string::npos);

    assert(json.find("\"counts\":{") != std::string::npos);
    assert(json.find("\"extendedEpgInfo\":1") != std::string::npos);
    assert(json.find("\"channelGroups\":1") != std::string::npos);
    assert(json.find("\"blacklists\":1") != std::string::npos);
    assert(json.find("\"recordingDirectories\":1") != std::string::npos);

    assert(json.find("\"extendedEpgInfo\":[{") != std::string::npos);
    assert(json.find("\"id\":7") != std::string::npos);
    assert(json.find("\"name\":\"Category \\\"HD\\\"\"") != std::string::npos);
    assert(json.find("\"values\":[\"Movie\",\"Series\"]") != std::string::npos);
    assert(json.find("\"config\":\"7|Category|Movie,Series\"") != std::string::npos);

    assert(json.find("\"channelGroups\":[{\"name\":\"HD Channels\"}]") != std::string::npos);
    assert(json.find("\"blacklists\":[{\"id\":4,\"search\":\"Teleshopping\"}]") != std::string::npos);
    assert(json.find("\"recordingDirectories\":[{\"path\":\"Doku\\\\Natur\"}]") != std::string::npos);

    std::cout
        << "test_search_timer_discovery_json_serializer passed"
        << std::endl;

    return 0;
}

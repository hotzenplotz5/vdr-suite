#include "SearchTimerDiscoveryService.h"

#include "ISearchTimerDiscoveryProvider.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{
class StaticDiscoveryProvider final
    : public ISearchTimerDiscoveryProvider
{
public:
    SearchTimerDiscoveryCatalog discover(
        const std::string& backendId) const override
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
};
}

int main()
{
    StaticDiscoveryProvider provider;
    SearchTimerDiscoveryService service(provider);

    SearchTimerDiscoveryCatalog catalog =
        service.discover("livingroom");

    assert(catalog.backendId() == "livingroom");
    assert(!catalog.empty());
    assert(catalog.extendedEpgInfoCount() == 1);
    assert(catalog.channelGroupCount() == 1);
    assert(catalog.blacklistCount() == 1);
    assert(catalog.recordingDirectoryCount() == 1);

    assert(catalog.extendedEpgInfos()[0].id() == 7);
    assert(catalog.extendedEpgInfos()[0].name() == "Category");
    assert(catalog.extendedEpgInfos()[0].values().size() == 2);
    assert(catalog.extendedEpgInfos()[0].values()[0] == "Movie");
    assert(catalog.extendedEpgInfos()[0].values()[1] == "Series");
    assert(catalog.channelGroups()[0].name() == "HD Channels");
    assert(catalog.blacklists()[0].id() == 4);
    assert(catalog.blacklists()[0].search() == "Teleshopping");
    assert(catalog.recordingDirectories()[0].path() == "Doku/Natur");

    SearchTimerDiscoveryCatalog bedroom =
        service.discover("bedroom");

    assert(bedroom.backendId() == "bedroom");
    assert(bedroom.extendedEpgInfoCount() == 1);

    std::cout
        << "test_search_timer_discovery_service passed"
        << std::endl;

    return 0;
}

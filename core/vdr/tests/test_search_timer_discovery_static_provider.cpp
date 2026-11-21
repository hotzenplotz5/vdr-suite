#include "SearchTimerDiscoveryStaticProvider.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerDiscoveryStaticProvider provider;

    SearchTimerDiscoveryCatalog catalog =
        provider.discover("default");

    assert(catalog.backendId() == "default");
    assert(catalog.empty());
    assert(catalog.extendedEpgInfoCount() == 0);
    assert(catalog.channelGroupCount() == 0);
    assert(catalog.blacklistCount() == 0);
    assert(catalog.recordingDirectoryCount() == 0);
    assert(catalog.extendedEpgInfos().empty());
    assert(catalog.channelGroups().empty());
    assert(catalog.blacklists().empty());
    assert(catalog.recordingDirectories().empty());

    SearchTimerDiscoveryCatalog remote =
        provider.discover("ferienhaus");

    assert(remote.backendId() == "ferienhaus");
    assert(remote.empty());

    std::cout
        << "test_search_timer_discovery_static_provider passed"
        << std::endl;

    return 0;
}

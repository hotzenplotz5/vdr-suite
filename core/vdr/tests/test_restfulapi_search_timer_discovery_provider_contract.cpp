#include "RestfulApiSearchTimerDiscoveryProvider.h"

#include <cassert>
#include <iostream>

int main()
{
    RestfulApiSearchTimerDiscoveryProvider provider("default");

    assert(provider.configuredBackendId() == "default");
    assert(provider.discoveryEndpoint() == "/searchtimers/discovery.json");

    SearchTimerDiscoveryCatalog defaultCatalog =
        provider.discover("");

    assert(defaultCatalog.backendId() == "default");
    assert(defaultCatalog.empty());
    assert(defaultCatalog.extendedEpgInfoCount() == 0);
    assert(defaultCatalog.channelGroupCount() == 0);
    assert(defaultCatalog.blacklistCount() == 0);
    assert(defaultCatalog.recordingDirectoryCount() == 0);

    SearchTimerDiscoveryCatalog remoteCatalog =
        provider.discover("ferienhaus");

    assert(remoteCatalog.backendId() == "ferienhaus");
    assert(remoteCatalog.empty());

    RestfulApiSearchTimerDiscoveryProvider customProvider(
        "living-room",
        "/custom/searchtimers/discovery.json");

    assert(customProvider.configuredBackendId() == "living-room");
    assert(customProvider.discoveryEndpoint() == "/custom/searchtimers/discovery.json");

    SearchTimerDiscoveryCatalog customCatalog =
        customProvider.discover("");

    assert(customCatalog.backendId() == "living-room");
    assert(customCatalog.empty());

    std::cout
        << "test_restfulapi_search_timer_discovery_provider_contract passed"
        << std::endl;

    return 0;
}

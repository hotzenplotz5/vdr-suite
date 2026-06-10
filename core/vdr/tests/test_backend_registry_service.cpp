#include "BackendRegistryService.h"

#include <cassert>

int main()
{
    BackendRegistry registry;

    BackendNode defaultBackend;
    defaultBackend.backendId = "default";
    defaultBackend.backendName = "Default VDR";

    BackendNode remoteBackend;
    remoteBackend.backendId = "remote";
    remoteBackend.backendName = "Remote VDR";

    BackendRegistryService emptyService(registry);

    assert(!emptyService.hasBackend("default"));
    assert(!emptyService.defaultBackend().has_value());
    assert(emptyService.listBackends().empty());

    registry.addBackend(defaultBackend);
    registry.addBackend(remoteBackend);

    BackendRegistryService service(registry);

    assert(service.hasBackend("default"));
    assert(service.hasBackend("remote"));
    assert(!service.hasBackend("missing"));

    const auto defaultLookup = service.defaultBackend();
    assert(defaultLookup.has_value());
    assert(defaultLookup->backendId == "default");
    assert(defaultLookup->backendName == "Default VDR");

    const auto remoteLookup = service.getBackend("remote");
    assert(remoteLookup.has_value());
    assert(remoteLookup->backendId == "remote");
    assert(remoteLookup->backendName == "Remote VDR");

    const auto allBackends = service.listBackends();
    assert(allBackends.size() == 2);
    assert(allBackends[0].backendId == "default");
    assert(allBackends[1].backendId == "remote");

    return 0;
}

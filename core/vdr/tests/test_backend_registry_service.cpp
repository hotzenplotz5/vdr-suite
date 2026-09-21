#include "BackendRegistryService.h"

#include "BackendAccessPolicy.h"

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
    remoteBackend.accessMode = "read-only";

    BackendRegistryService emptyService(registry);
    BackendAccessPolicy accessPolicy;

    assert(!emptyService.hasBackend("default"));
    assert(!emptyService.defaultBackend().has_value());
    assert(emptyService.listBackends().empty());

    const BackendAccessDecision emptyBackendDecision =
        accessPolicy.canWriteToBackend(emptyService, "default");

    assert(!emptyBackendDecision.allowed);
    assert(emptyBackendDecision.reason == "backend not found");

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
    assert(remoteLookup->readOnly());

    const auto allBackends = service.listBackends();
    assert(allBackends.size() == 2);
    assert(allBackends[0].backendId == "default");
    assert(allBackends[1].backendId == "remote");

    const BackendAccessDecision defaultDecision =
        accessPolicy.canWriteToBackend(service, "default");

    assert(defaultDecision.allowed);
    assert(defaultDecision.backendFound);
    assert(defaultDecision.reason == "backend write access allowed");

    const BackendAccessDecision remoteDecision =
        accessPolicy.canWriteToBackend(service, "remote");

    assert(!remoteDecision.allowed);
    assert(remoteDecision.backendFound);
    assert(remoteDecision.readOnly);
    assert(remoteDecision.reason == "backend is read-only");
    assert(remoteDecision.errors.size() == 1);

    return 0;
}

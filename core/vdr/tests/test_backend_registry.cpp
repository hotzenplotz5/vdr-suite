#include "BackendRegistry.h"

#include <cassert>

int main()
{
    BackendRegistry registry;

    BackendNode backend;
    backend.backendId = "wohnzimmer";

    assert(!registry.hasBackend("wohnzimmer"));

    registry.addBackend(backend);

    assert(registry.hasBackend("wohnzimmer"));
    assert(!registry.hasBackend("ferienhaus"));

    return 0;
}

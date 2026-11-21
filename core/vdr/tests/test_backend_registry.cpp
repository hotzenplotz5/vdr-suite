#include "BackendRegistry.h"

#include <cassert>

int main()
{
    BackendRegistry registry;

    BackendNode wohnzimmer;
    wohnzimmer.backendId = "wohnzimmer";
    wohnzimmer.backendName = "Wohnzimmer VDR";

    BackendNode ferienhaus;
    ferienhaus.backendId = "ferienhaus";
    ferienhaus.backendName = "Ferienhaus VDR";

    assert(!registry.hasBackend("wohnzimmer"));
    assert(!registry.getBackend("wohnzimmer").has_value());
    assert(registry.listBackends().empty());

    registry.addBackend(wohnzimmer);

    assert(registry.hasBackend("wohnzimmer"));
    assert(!registry.hasBackend("ferienhaus"));

    const auto firstLookup = registry.getBackend("wohnzimmer");
    assert(firstLookup.has_value());
    assert(firstLookup->backendId == "wohnzimmer");
    assert(firstLookup->backendName == "Wohnzimmer VDR");

    const auto missingLookup = registry.getBackend("ferienhaus");
    assert(!missingLookup.has_value());

    registry.addBackend(ferienhaus);

    assert(registry.hasBackend("ferienhaus"));

    const auto allBackends = registry.listBackends();
    assert(allBackends.size() == 2);
    assert(allBackends[0].backendId == "wohnzimmer");
    assert(allBackends[1].backendId == "ferienhaus");

    return 0;
}

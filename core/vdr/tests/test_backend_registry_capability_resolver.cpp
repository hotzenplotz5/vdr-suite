#include "BackendRegistry.h"
#include "BackendRegistryCapabilityResolver.h"
#include "BackendRegistryService.h"
#include "CapabilityState.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

namespace
{
BackendNode makeDefaultBackend()
{
    BackendNode backend;
    backend.backendId = "default";
    backend.backendName = "Default VDR";
    backend.backendType = "restfulapi";
    backend.enabled = true;
    backend.online = true;
    backend.capabilities = VdrCapabilitySet::snapshotReadOnly();
    return backend;
}

void resolverReflectsBackendRegistryCapabilityUpdates()
{
    BackendRegistry registry;
    registry.addBackend(makeDefaultBackend());

    BackendRegistryService backendRegistryService(registry);
    BackendRegistryCapabilityResolver resolver(
        backendRegistryService,
        "default");

    const auto initialNative =
        resolver.state("epg.search.fuzzy.native");

    assert(!initialNative.supported());
    assert(!initialNative.availableNow());
    assert(initialNative.availability() == CapabilityAvailability::Unsupported);

    VdrCapabilitySet refreshedCapabilities =
        VdrCapabilitySet::snapshotReadOnly();
    refreshedCapabilities.epgSearchFuzzyNative = true;

    assert(
        backendRegistryService.updateBackendCapabilities(
            "default",
            refreshedCapabilities));

    const auto refreshedNative =
        resolver.state("epg.search.fuzzy.native");

    assert(refreshedNative.supported());
    assert(refreshedNative.availableNow());
    assert(refreshedNative.availability() == CapabilityAvailability::Available);
    assert(resolver.supports("epg.search.fuzzy.native"));
}

void resolverFailsSafelyForMissingBackend()
{
    BackendRegistry registry;
    BackendRegistryService backendRegistryService(registry);
    BackendRegistryCapabilityResolver resolver(
        backendRegistryService,
        "missing");

    const auto state =
        resolver.state("epg.search.fuzzy.native");

    assert(!state.supported());
    assert(!state.availableNow());
    assert(state.availability() == CapabilityAvailability::Unsupported);
    assert(state.reason() == "backend not found: missing");
}
}

int main()
{
    resolverReflectsBackendRegistryCapabilityUpdates();
    resolverFailsSafelyForMissingBackend();

    std::cout
        << "test_backend_registry_capability_resolver passed"
        << std::endl;

    return 0;
}

#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "CapabilityResolver.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

static BackendNode makeBackend(
    const std::string& backendId)
{
    BackendNode backend;
    backend.backendId = backendId;
    backend.backendName = backendId + " VDR";
    backend.capabilities = VdrCapabilitySet::snapshotReadOnly();
    backend.enabled = true;
    backend.online = true;

    return backend;
}

static void assertNativeFuzzyCapability(
    const BackendNode& backend,
    bool expected)
{
    CapabilityResolver resolver(backend.capabilities);

    assert(resolver.supports("epg.search.fuzzy.fallback"));
    assert(resolver.supports("epg.search.fuzzy.native") == expected);
}

static void test_successful_probe_updates_backend_capability()
{
    BackendRegistry registry;
    registry.addBackend(makeBackend("default"));

    BackendRegistryService service(registry);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    const auto baseBackend = service.getBackend("default");
    assert(baseBackend.has_value());
    assertNativeFuzzyCapability(*baseBackend, false);

    const VdrCapabilitySet detectedCapabilities = detector.apply(
        baseBackend->capabilities,
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip());

    const bool updated = service.updateBackendCapabilities(
        "default",
        detectedCapabilities);

    assert(updated);

    const auto updatedBackend = service.getBackend("default");
    assert(updatedBackend.has_value());
    assertNativeFuzzyCapability(*updatedBackend, true);
    assert(updatedBackend->capabilities.epgSearchFuzzyNative);
    assert(updatedBackend->capabilities.epgSearchFuzzyFallback);
}

static void test_failed_probe_keeps_native_fuzzy_disabled()
{
    BackendRegistry registry;
    registry.addBackend(makeBackend("default"));

    BackendRegistryService service(registry);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    const auto baseBackend = service.getBackend("default");
    assert(baseBackend.has_value());

    EpgSearchNativeFuzzyCapabilityProbeResult failedResult =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();
    failedResult.cleanupSucceeded = false;

    const VdrCapabilitySet detectedCapabilities = detector.apply(
        baseBackend->capabilities,
        failedResult);

    const bool updated = service.updateBackendCapabilities(
        "default",
        detectedCapabilities);

    assert(updated);

    const auto updatedBackend = service.getBackend("default");
    assert(updatedBackend.has_value());
    assertNativeFuzzyCapability(*updatedBackend, false);
    assert(!updatedBackend->capabilities.epgSearchFuzzyNative);
    assert(updatedBackend->capabilities.epgSearchFuzzyFallback);
}

static void test_missing_backend_is_not_created_by_capability_update()
{
    BackendRegistry registry;
    BackendRegistryService service(registry);

    const bool updated = service.updateBackendCapabilities(
        "missing",
        VdrCapabilitySet::snapshotReadOnly());

    assert(!updated);
    assert(!service.hasBackend("missing"));
    assert(service.listBackends().empty());
}

static void test_capability_update_preserves_backend_identity()
{
    BackendRegistry registry;
    registry.addBackend(makeBackend("living-room"));

    BackendRegistryService service(registry);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    const auto baseBackend = service.getBackend("living-room");
    assert(baseBackend.has_value());

    const VdrCapabilitySet detectedCapabilities = detector.apply(
        baseBackend->capabilities,
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip());

    assert(service.updateBackendCapabilities("living-room", detectedCapabilities));

    const auto updatedBackend = service.getBackend("living-room");
    assert(updatedBackend.has_value());
    assert(updatedBackend->backendId == "living-room");
    assert(updatedBackend->backendName == "living-room VDR");
    assert(updatedBackend->enabled);
    assert(updatedBackend->online);
    assertNativeFuzzyCapability(*updatedBackend, true);
}

int main()
{
    test_successful_probe_updates_backend_capability();
    test_failed_probe_keeps_native_fuzzy_disabled();
    test_missing_backend_is_not_created_by_capability_update();
    test_capability_update_preserves_backend_identity();

    std::cout
        << "test_epgsearch_native_fuzzy_runtime_capability_wiring passed"
        << std::endl;

    return 0;
}

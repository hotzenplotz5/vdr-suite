#include "CapabilityResolver.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

static void assertNativeCapability(
    const VdrCapabilitySet& capabilities,
    bool expected)
{
    CapabilityResolver resolver(capabilities);

    assert(resolver.supports("epg.search.fuzzy.fallback"));
    assert(resolver.supports("epg.search.fuzzy.native") == expected);

    const CapabilityState state =
        resolver.state("epg.search.fuzzy.native");

    assert(state.capabilityName() == "epg.search.fuzzy.native");
    assert(state.supported() == expected);
    assert(state.availableNow() == expected);
}

static void test_successful_probe_enables_native_capability()
{
    const VdrCapabilitySet baseCapabilities =
        VdrCapabilitySet::snapshotReadOnly();

    EpgSearchNativeFuzzyCapabilityDetector detector;

    const VdrCapabilitySet detectedCapabilities = detector.apply(
        baseCapabilities,
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip());

    assert(baseCapabilities.epgSearchFuzzyFallback);
    assert(!baseCapabilities.epgSearchFuzzyNative);
    assert(detectedCapabilities.epgSearchFuzzyFallback);
    assert(detectedCapabilities.epgSearchFuzzyNative);

    assertNativeCapability(detectedCapabilities, true);
}

static void test_failed_probe_keeps_native_capability_disabled()
{
    EpgSearchNativeFuzzyCapabilityDetector detector;

    EpgSearchNativeFuzzyCapabilityProbeResult result =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();
    result.tolerancePreserved = false;

    const VdrCapabilitySet detectedCapabilities = detector.apply(
        VdrCapabilitySet::snapshotReadOnly(),
        result);

    assert(detectedCapabilities.epgSearchFuzzyFallback);
    assert(!detectedCapabilities.epgSearchFuzzyNative);

    assertNativeCapability(detectedCapabilities, false);
}

static void test_cleanup_failure_keeps_native_capability_disabled()
{
    EpgSearchNativeFuzzyCapabilityDetector detector;

    EpgSearchNativeFuzzyCapabilityProbeResult result =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();
    result.cleanupSucceeded = false;

    const VdrCapabilitySet detectedCapabilities = detector.apply(
        VdrCapabilitySet::snapshotReadOnly(),
        result);

    assert(detectedCapabilities.epgSearchFuzzyFallback);
    assert(!detectedCapabilities.epgSearchFuzzyNative);

    assertNativeCapability(detectedCapabilities, false);
}

static void test_empty_probe_keeps_existing_base_capabilities()
{
    EpgSearchNativeFuzzyCapabilityDetector detector;

    VdrCapabilitySet baseCapabilities =
        VdrCapabilitySet::snapshotReadOnly();
    baseCapabilities.channelsRead = false;

    EpgSearchNativeFuzzyCapabilityProbeResult result;

    const VdrCapabilitySet detectedCapabilities = detector.apply(
        baseCapabilities,
        result);

    assert(detectedCapabilities.epgSearchFuzzyFallback);
    assert(!detectedCapabilities.epgSearchFuzzyNative);
    assert(!detectedCapabilities.channelsRead);
}

int main()
{
    test_successful_probe_enables_native_capability();
    test_failed_probe_keeps_native_capability_disabled();
    test_cleanup_failure_keeps_native_capability_disabled();
    test_empty_probe_keeps_existing_base_capabilities();

    std::cout
        << "test_epgsearch_native_fuzzy_capability_detector passed"
        << std::endl;

    return 0;
}

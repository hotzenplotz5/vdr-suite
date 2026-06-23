#pragma once

#include "VdrCapabilitySet.h"

struct EpgSearchNativeFuzzyCapabilityProbeResult
{
    bool createAccepted = false;
    bool readbackAvailable = false;
    bool modePreserved = false;
    bool tolerancePreserved = false;
    bool cleanupSucceeded = false;
};

class EpgSearchNativeFuzzyCapabilityDetector
{
public:
    bool nativeFuzzyAvailable(
        const EpgSearchNativeFuzzyCapabilityProbeResult& result) const
    {
        return result.createAccepted
            && result.readbackAvailable
            && result.modePreserved
            && result.tolerancePreserved
            && result.cleanupSucceeded;
    }

    VdrCapabilitySet apply(
        const VdrCapabilitySet& baseCapabilities,
        const EpgSearchNativeFuzzyCapabilityProbeResult& result) const
    {
        VdrCapabilitySet capabilities = baseCapabilities;
        capabilities.epgSearchFuzzyNative = nativeFuzzyAvailable(result);
        return capabilities;
    }

    static EpgSearchNativeFuzzyCapabilityProbeResult successfulRoundTrip()
    {
        EpgSearchNativeFuzzyCapabilityProbeResult result;
        result.createAccepted = true;
        result.readbackAvailable = true;
        result.modePreserved = true;
        result.tolerancePreserved = true;
        result.cleanupSucceeded = true;
        return result;
    }
};

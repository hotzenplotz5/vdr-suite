#pragma once

#include "CapabilityState.h"
#include "ICapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <string>

class CapabilityResolver : public ICapabilityResolver
{
public:
    explicit CapabilityResolver(
        const VdrCapabilitySet& capabilities)
        : capabilities_(capabilities)
    {
    }

    bool supports(
        const std::string& capability) const override
    {
        return state(capability).availableNow();
    }

    CapabilityState state(
        const std::string& capability) const override
    {
        if (capability == "snapshot.read")
        {
            return fromSupportedFlag(capability, capabilities_.snapshotRead);
        }

        if (capability == "status.read")
        {
            return fromSupportedFlag(capability, capabilities_.statusRead);
        }

        if (capability == "health.read")
        {
            return fromSupportedFlag(capability, capabilities_.healthRead);
        }

        if (capability == "recordings.read")
        {
            return fromSupportedFlag(capability, capabilities_.recordingsRead);
        }

        if (capability == "timers.read")
        {
            return fromSupportedFlag(capability, capabilities_.timersRead);
        }

        if (capability == "channels.read")
        {
            return fromSupportedFlag(capability, capabilities_.channelsRead);
        }

        if (capability == "events.read")
        {
            return fromSupportedFlag(capability, capabilities_.eventsRead);
        }

        if (capability == "events.read.selective")
        {
            return fromSupportedFlag(capability, capabilities_.eventsSelectiveRead);
        }

        if (capability == "epg.search.fuzzy.fallback")
        {
            return fromSupportedFlag(capability, capabilities_.epgSearchFuzzyFallback);
        }

        if (capability == "epg.search.fuzzy.native")
        {
            return fromSupportedFlag(capability, capabilities_.epgSearchFuzzyNative);
        }

        if (capability == "searchtimer.preview.native")
        {
            return fromSupportedFlag(capability, capabilities_.searchTimerPreviewNative);
        }

        return CapabilityState::unsupported(
            capability,
            "unknown capability");
    }

private:
    static CapabilityState fromSupportedFlag(
        const std::string& capability,
        bool supported)
    {
        if (supported)
        {
            return CapabilityState::available(capability);
        }

        return CapabilityState::unsupported(
            capability,
            "capability unsupported by backend");
    }

    const VdrCapabilitySet& capabilities_;
};

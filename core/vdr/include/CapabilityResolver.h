#pragma once

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
        if (capability == "snapshot.read")
        {
            return capabilities_.snapshotRead;
        }

        if (capability == "status.read")
        {
            return capabilities_.statusRead;
        }

        if (capability == "health.read")
        {
            return capabilities_.healthRead;
        }

        if (capability == "recordings.read")
        {
            return capabilities_.recordingsRead;
        }

        if (capability == "timers.read")
        {
            return capabilities_.timersRead;
        }

        if (capability == "channels.read")
        {
            return capabilities_.channelsRead;
        }

        if (capability == "events.read")
        {
            return capabilities_.eventsRead;
        }

        if (capability == "events.read.selective")
        {
            return capabilities_.eventsSelectiveRead;
        }

        return false;
    }

private:
    const VdrCapabilitySet& capabilities_;
};

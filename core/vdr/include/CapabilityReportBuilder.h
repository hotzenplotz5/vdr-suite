#pragma once

#include "CapabilityReport.h"
#include "ICapabilityResolver.h"

#include <string>
#include <vector>

class CapabilityReportBuilder
{
public:
    CapabilityReport build(
        const std::string& backendId,
        const ICapabilityResolver& resolver) const
    {
        std::vector<CapabilityState> states;

        for (const auto& capability : defaultCapabilities())
        {
            states.push_back(resolver.state(capability));
        }

        return CapabilityReport(
            backendId,
            states);
    }

private:
    static const std::vector<std::string>& defaultCapabilities()
    {
        static const std::vector<std::string> capabilities = {
            "snapshot.read",
            "status.read",
            "health.read",
            "recordings.read",
            "timers.read",
            "channels.read",
            "events.read",
            "events.read.selective"
        };

        return capabilities;
    }
};

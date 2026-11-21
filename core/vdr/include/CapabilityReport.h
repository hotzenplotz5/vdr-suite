#pragma once

#include "CapabilityState.h"

#include <string>
#include <utility>
#include <vector>

class CapabilityReport
{
public:
    CapabilityReport(
        std::string backendId,
        std::vector<CapabilityState> capabilities)
        : backendId_(std::move(backendId)),
          capabilities_(std::move(capabilities))
    {
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    const std::vector<CapabilityState>& capabilities() const
    {
        return capabilities_;
    }

    bool empty() const
    {
        return capabilities_.empty();
    }

    std::size_t size() const
    {
        return capabilities_.size();
    }

private:
    std::string backendId_;
    std::vector<CapabilityState> capabilities_;
};

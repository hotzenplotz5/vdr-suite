#pragma once

#include "CapabilityState.h"

#include <string>

class CapabilityStateJsonSerializer
{
public:
    std::string serialize(
        const CapabilityState& state) const;
};

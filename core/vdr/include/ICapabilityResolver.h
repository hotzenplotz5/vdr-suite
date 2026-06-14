#pragma once

#include "CapabilityState.h"

#include <string>

class ICapabilityResolver
{
public:
    virtual ~ICapabilityResolver() = default;

    virtual bool supports(
        const std::string& capability) const = 0;

    virtual CapabilityState state(
        const std::string& capability) const = 0;
};

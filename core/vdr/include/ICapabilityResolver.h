#pragma once

#include <string>

class ICapabilityResolver
{
public:
    virtual ~ICapabilityResolver() = default;

    virtual bool supports(
        const std::string& capability) const = 0;
};

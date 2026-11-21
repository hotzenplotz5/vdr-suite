#pragma once

#include "PersonResolver.h"

#include <string>

class PersonResolutionJsonSerializer
{
public:
    std::string serialize(
        const PersonResolutionResult& result) const;
};

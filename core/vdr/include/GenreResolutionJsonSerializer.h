#pragma once

#include "GenreResolver.h"

#include <string>

class GenreResolutionJsonSerializer
{
public:
    std::string serialize(
        const GenreResolutionResult& result) const;
};

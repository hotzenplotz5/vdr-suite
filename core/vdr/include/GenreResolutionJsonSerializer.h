#pragma once

#include "GenreResolver.h"

#include <string>

class GenreResolutionJsonSerializer
{
public:
    std::string serialize(
        const GenreResolutionResult& result) const;

    std::string serializeLocalized(
        const GenreResolutionResult& result,
        const std::string& locale) const;
};

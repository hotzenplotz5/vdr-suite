#pragma once

#include "ContentRatingResolver.h"

#include <string>

class ContentRatingResolutionJsonSerializer
{
public:
    std::string serialize(
        const ContentRatingResolutionResult& result) const;
};

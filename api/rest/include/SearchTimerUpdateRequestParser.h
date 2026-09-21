#pragma once

#include "SearchTimerUpdateRequest.h"

#include <string>

class SearchTimerUpdateRequestParser
{
public:
    SearchTimerUpdateRequest parse(
        const std::string& body) const;
};

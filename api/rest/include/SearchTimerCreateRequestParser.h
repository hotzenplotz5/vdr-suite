#pragma once

#include "SearchTimerCreateRequest.h"

#include <string>

class SearchTimerCreateRequestParser
{
public:
    SearchTimerCreateRequest parse(
        const std::string& body) const;
};

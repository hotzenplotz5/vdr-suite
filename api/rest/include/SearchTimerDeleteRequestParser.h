#pragma once

#include "SearchTimerDeleteRequest.h"

#include <string>

class SearchTimerDeleteRequestParser
{
public:
    SearchTimerDeleteRequest parse(
        const std::string& body) const;
};

#pragma once

#include "VdrTimerOperationRequest.h"

#include <string>

class VdrTimerActionRequestParser
{
public:
    VdrTimerOperationRequest parse(
        const std::string& body) const;
};

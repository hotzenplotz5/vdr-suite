#pragma once

#include "VdrTimerActionResult.h"

#include <string>

class VdrTimerActionResultJsonSerializer
{
public:
    std::string serialize(
        const VdrTimerActionResult& result) const;
};

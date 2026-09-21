#pragma once

#include "VdrTimerConflict.h"

#include <string>

class RestfulApiTimerConflictMapper
{
public:
    static VdrTimerConflictReport parseReport(
        const std::string& json,
        int statusCode);
};

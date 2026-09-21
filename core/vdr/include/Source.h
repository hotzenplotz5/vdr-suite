#pragma once

#include <string>
#include "SourceType.h"

struct Source
{
    std::string id;
    std::string name;
    SourceType type = SourceType::Unknown;
    bool enabled = true;
};

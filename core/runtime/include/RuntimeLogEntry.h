#pragma once

#include "RuntimeLogLevel.h"

#include <string>

struct RuntimeLogEntry {
    RuntimeLogLevel level;
    std::string component;
    std::string text;
};

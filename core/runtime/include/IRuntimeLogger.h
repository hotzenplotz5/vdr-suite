#pragma once

#include "RuntimeLogEntry.h"

class IRuntimeLogger {
public:
    virtual ~IRuntimeLogger() = default;

    virtual void write(const RuntimeLogEntry& entry) = 0;
};

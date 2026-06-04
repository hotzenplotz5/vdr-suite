#pragma once

#include "RuntimeLogEntry.h"

class RuntimeLogger {
public:
    virtual ~RuntimeLogger() = default;

    virtual void write(const RuntimeLogEntry& entry) = 0;
};

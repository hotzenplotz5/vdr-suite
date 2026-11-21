#pragma once

#include "IRuntimeLogger.h"

class NullRuntimeLogger : public IRuntimeLogger {
public:
    void write(const RuntimeLogEntry& entry) override;
};

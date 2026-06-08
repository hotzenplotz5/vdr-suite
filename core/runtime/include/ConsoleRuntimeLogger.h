#pragma once

#include "IRuntimeLogger.h"

class ConsoleRuntimeLogger : public IRuntimeLogger {
public:
    void write(const RuntimeLogEntry& entry) override;
};

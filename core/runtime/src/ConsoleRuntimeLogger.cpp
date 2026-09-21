#include "ConsoleRuntimeLogger.h"

#include <iostream>

namespace {

const char* toString(RuntimeLogLevel level)
{
    switch (level) {
        case RuntimeLogLevel::Debug:
            return "DEBUG";

        case RuntimeLogLevel::Info:
            return "INFO";

        case RuntimeLogLevel::Warning:
            return "WARNING";

        case RuntimeLogLevel::Error:
            return "ERROR";
    }

    return "UNKNOWN";
}

}

void ConsoleRuntimeLogger::write(const RuntimeLogEntry& entry)
{
    std::cout
        << "["
        << toString(entry.level)
        << "] "
        << "["
        << entry.component
        << "] "
        << entry.text
        << std::endl;
}

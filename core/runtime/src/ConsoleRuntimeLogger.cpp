#include "ConsoleRuntimeLogger.h"

#include <iostream>

void ConsoleRuntimeLogger::write(const RuntimeLogEntry& entry)
{
    std::cout
        << "["
        << entry.component
        << "] "
        << entry.text
        << std::endl;
}

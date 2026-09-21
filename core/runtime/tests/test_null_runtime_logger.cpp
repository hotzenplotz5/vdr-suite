#include "NullRuntimeLogger.h"

int main()
{
    NullRuntimeLogger logger;
    RuntimeLogEntry entry{RuntimeLogLevel::Info, "test", "message"};

    logger.write(entry);

    return 0;
}

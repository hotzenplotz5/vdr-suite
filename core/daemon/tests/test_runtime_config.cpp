#include "RuntimeConfig.h"

#include <cassert>
#include <cstdlib>
#include <string>

int main()
{
    unsetenv("VDR_SUITE_DATABASE_PATH");

    RuntimeConfig defaultConfig;
    assert(defaultConfig.databasePath() == "/tmp/vdr-suite-test.db");

    setenv("VDR_SUITE_DATABASE_PATH", "/var/lib/vdr-suite/vdr-suite.db", 1);

    RuntimeConfig overriddenConfig;
    assert(overriddenConfig.databasePath() == "/var/lib/vdr-suite/vdr-suite.db");

    setenv("VDR_SUITE_DATABASE_PATH", "", 1);

    RuntimeConfig emptyOverrideConfig;
    assert(emptyOverrideConfig.databasePath() == "/tmp/vdr-suite-test.db");

    unsetenv("VDR_SUITE_DATABASE_PATH");

    return 0;
}

#include "RuntimeConfig.h"

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>

int main()
{
    unsetenv("VDR_SUITE_DATABASE_PATH");
    unsetenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS");

    RuntimeConfig defaultConfig;
    assert(defaultConfig.databasePath() == "/tmp/vdr-suite-test.db");
    assert(defaultConfig.recordingArtworkRoots().size() == 1);
    assert(defaultConfig.recordingArtworkRoots().front() ==
           "/var/cache/vdr/plugins/tvscraper");

    setenv(
        "VDR_SUITE_DATABASE_PATH",
        "/var/lib/vdr-suite/vdr-suite.db",
        1);
    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "/srv/tvscraper:/mnt/secondary-artwork",
        1);

    RuntimeConfig overriddenConfig;
    assert(overriddenConfig.databasePath() ==
           "/var/lib/vdr-suite/vdr-suite.db");
    assert(overriddenConfig.recordingArtworkRoots().size() == 2);
    assert(overriddenConfig.recordingArtworkRoots().at(0) ==
           "/srv/tvscraper");
    assert(overriddenConfig.recordingArtworkRoots().at(1) ==
           "/mnt/secondary-artwork");

    setenv("VDR_SUITE_DATABASE_PATH", "", 1);
    setenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS", "", 1);

    RuntimeConfig emptyOverrideConfig;
    assert(emptyOverrideConfig.databasePath() ==
           "/tmp/vdr-suite-test.db");
    assert(emptyOverrideConfig.recordingArtworkRoots().size() == 1);
    assert(emptyOverrideConfig.recordingArtworkRoots().front() ==
           "/var/cache/vdr/plugins/tvscraper");

    unsetenv("VDR_SUITE_DATABASE_PATH");
    unsetenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS");

    return 0;
}

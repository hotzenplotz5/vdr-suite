#include "RuntimeConfig.h"

#include <cassert>
#include <cstdlib>
#include <string>

int main()
{
    unsetenv("VDR_SUITE_DATABASE_PATH");
    unsetenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS");

    RuntimeConfig defaultConfig;
    assert(defaultConfig.databasePath() == "/tmp/vdr-suite-test.db");
    assert(defaultConfig.recordingArtworkRoots().empty());

    setenv(
        "VDR_SUITE_DATABASE_PATH",
        "/var/lib/vdr-suite/vdr-suite.db",
        1);
    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=/srv/tvscraper;wohnhaus2=/mnt/secondary artwork",
        1);

    RuntimeConfig overriddenConfig;
    assert(overriddenConfig.databasePath() ==
           "/var/lib/vdr-suite/vdr-suite.db");
    assert(overriddenConfig.recordingArtworkRoots().size() == 2);
    assert(overriddenConfig.recordingArtworkRoots().at("default") ==
           "/srv/tvscraper");
    assert(overriddenConfig.recordingArtworkRoots().at("wohnhaus2") ==
           "/mnt/secondary artwork");

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=/srv/one;default=/srv/two",
        1);
    RuntimeConfig duplicateConfig;
    assert(duplicateConfig.recordingArtworkRoots().empty());

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=relative/path",
        1);
    RuntimeConfig relativeConfig;
    assert(relativeConfig.recordingArtworkRoots().empty());

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "invalid backend=/srv/tvscraper",
        1);
    RuntimeConfig invalidBackendConfig;
    assert(invalidBackendConfig.recordingArtworkRoots().empty());

    setenv("VDR_SUITE_DATABASE_PATH", "", 1);
    setenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS", "", 1);

    RuntimeConfig emptyOverrideConfig;
    assert(emptyOverrideConfig.databasePath() ==
           "/tmp/vdr-suite-test.db");
    assert(emptyOverrideConfig.recordingArtworkRoots().empty());

    unsetenv("VDR_SUITE_DATABASE_PATH");
    unsetenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS");

    return 0;
}

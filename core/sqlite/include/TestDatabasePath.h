#pragma once

#include <cstdlib>
#include <string>

// Test-only database location. make supplies an absolute per-BUILD_DIR path;
// direct invocations from the repository root use its local build directory.
inline std::string vdrSuiteTestDatabasePath()
{
    const char* configured = std::getenv("VDR_SUITE_TEST_DATABASE_PATH");
    if (configured != nullptr && configured[0] != '\0')
        return configured;
    return ".build/vdr-suite-test.db";
}

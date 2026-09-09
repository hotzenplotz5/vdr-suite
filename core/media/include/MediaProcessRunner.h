#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <sys/types.h>
#include <vector>

struct MediaProcessCaptureResult
{
    bool started = false;
    bool completed = false;
    bool success = false;
    bool timedOut = false;
    bool outputLimitExceeded = false;
    int exitCode = -1;
    std::string reasonCode;
    std::string output;
};

// Zero leaves the corresponding limit unchanged for existing media callers.
struct MediaProcessLimits
{
    std::size_t addressSpaceBytes = 0;
    unsigned cpuSeconds = 0;
};

class MediaProcessRunner
{
public:
    MediaProcessCaptureResult runAndCapture(
        const std::vector<std::string>& argv,
        const std::string& workingDirectory,
        std::chrono::milliseconds timeout,
        std::size_t maximumOutputBytes,
        MediaProcessLimits limits = {}) const;

    pid_t spawnLogged(
        const std::vector<std::string>& argv,
        const std::string& workingDirectory,
        const std::string& logPath) const;

    bool terminateAndWait(
        pid_t pid,
        std::chrono::milliseconds gracePeriod) const;

private:
    static bool validInvocation(
        const std::vector<std::string>& argv,
        const std::string& workingDirectory);
};

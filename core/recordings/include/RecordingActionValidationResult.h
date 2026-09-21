#pragma once

#include <string>
#include <vector>

struct RecordingActionValidationResult
{
    bool valid = false;
    bool dryRun = true;
    bool wouldCreateJob = false;
    std::string backendId;
    std::string recordingId;
    std::vector<std::string> requiredCapabilities;
    std::vector<std::string> requiredPermissions;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

#pragma once

#include <string>

namespace vdrsuite::agent {

struct RecordingMoveNameResult
{
    bool success = false;
    std::string newName;
    std::string error;
};

RecordingMoveNameResult buildMovedRecordingName(
    const std::string& currentRecordingName,
    const std::string& targetFolder);

} // namespace vdrsuite::agent

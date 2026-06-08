#pragma once

#include <string>

enum class RecordingActionType
{
    Check,
    Repair,
    Shrink,
    Cut,
    Pes2Ts,
    Rename,
    Unknown
};

struct RecordingAction
{
    int recordingId = 0;

    RecordingActionType type = RecordingActionType::Unknown;

    std::string status;
    std::string message;
};

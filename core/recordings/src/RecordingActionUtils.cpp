#include "RecordingActionUtils.h"

std::string toString(
    RecordingActionType type)
{
    switch (type)
    {
        case RecordingActionType::Check:
            return "CHECK";

        case RecordingActionType::Repair:
            return "REPAIR";

        case RecordingActionType::Shrink:
            return "SHRINK";

        case RecordingActionType::Cut:
            return "CUT";

        case RecordingActionType::Pes2Ts:
            return "PES2TS";

        case RecordingActionType::Rename:
            return "RENAME";

        default:
            return "UNKNOWN";
    }
}

RecordingActionType fromString(
    const std::string& value)
{
    if (value == "CHECK")
        return RecordingActionType::Check;

    if (value == "REPAIR")
        return RecordingActionType::Repair;

    if (value == "SHRINK")
        return RecordingActionType::Shrink;

    if (value == "CUT")
        return RecordingActionType::Cut;

    if (value == "PES2TS")
        return RecordingActionType::Pes2Ts;

    if (value == "RENAME")
        return RecordingActionType::Rename;

    return RecordingActionType::Unknown;
}

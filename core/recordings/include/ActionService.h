#pragma once

#include "RecordingAction.h"

class ActionService
{
public:
    RecordingAction createAction(
        int recordingId,
        RecordingActionType type);
};

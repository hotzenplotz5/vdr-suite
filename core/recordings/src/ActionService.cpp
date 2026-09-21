#include "ActionService.h"

RecordingAction ActionService::createAction(
    int recordingId,
    RecordingActionType type)
{
    RecordingAction action;

    action.recordingId = recordingId;
    action.type = type;

    action.status = "PENDING";
    action.message = "";

    return action;
}

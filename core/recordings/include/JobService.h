#pragma once

#include "Job.h"
#include "RecordingAction.h"

class JobService
{
public:
    Job createJob(
        int recordingId,
        RecordingActionType actionType);
};

#pragma once

#include "Job.h"
#include "RecordingAction.h"

class ActionService;
class JobService;
class JobRepository;

class RecordingWorkflowService
{
public:
    RecordingWorkflowService(
        ActionService& actionService,
        JobService& jobService,
        JobRepository& jobRepository);

    Job createActionJob(
        int recordingId,
        RecordingActionType actionType);

private:
    ActionService& actionService_;
    JobService& jobService_;
    JobRepository& jobRepository_;
};

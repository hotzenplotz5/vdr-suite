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

    Job createCheckJob(int recordingId);
    Job createShrinkJob(int recordingId);
    Job createRepairJob(int recordingId);
    Job createCutJob(int recordingId);
    Job createPes2TsJob(int recordingId);
    Job createRenameJob(int recordingId);

private:
    ActionService& actionService_;
    JobService& jobService_;
    JobRepository& jobRepository_;
};

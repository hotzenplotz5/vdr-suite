#include "RecordingWorkflowService.h"

#include "ActionService.h"
#include "JobService.h"
#include "JobRepository.h"

RecordingWorkflowService::RecordingWorkflowService(
    ActionService& actionService,
    JobService& jobService,
    JobRepository& jobRepository)
    : actionService_(actionService),
      jobService_(jobService),
      jobRepository_(jobRepository)
{
}

Job RecordingWorkflowService::createActionJob(
    int recordingId,
    RecordingActionType actionType)
{
    auto action =
        actionService_.createAction(
            recordingId,
            actionType);

    auto job =
        jobService_.createJob(
            action.recordingId,
            action.type);

    jobRepository_.insertJob(job);

    return job;
}

Job RecordingWorkflowService::createCheckJob(int recordingId)
{
    return createActionJob(
        recordingId,
        RecordingActionType::Check);
}

Job RecordingWorkflowService::createShrinkJob(int recordingId)
{
    return createActionJob(
        recordingId,
        RecordingActionType::Shrink);
}

Job RecordingWorkflowService::createRepairJob(int recordingId)
{
    return createActionJob(
        recordingId,
        RecordingActionType::Repair);
}

Job RecordingWorkflowService::createCutJob(int recordingId)
{
    return createActionJob(
        recordingId,
        RecordingActionType::Cut);
}

Job RecordingWorkflowService::createPes2TsJob(int recordingId)
{
    return createActionJob(
        recordingId,
        RecordingActionType::Pes2Ts);
}

Job RecordingWorkflowService::createRenameJob(int recordingId)
{
    return createActionJob(
        recordingId,
        RecordingActionType::Rename);
}

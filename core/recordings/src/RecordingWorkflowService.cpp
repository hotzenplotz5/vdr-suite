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
    auto action = actionService_.createAction(recordingId, actionType);

    auto job = jobService_.createJob(
        action.recordingId,
        action.type);

    jobRepository_.insertJob(job);

    return job;
}

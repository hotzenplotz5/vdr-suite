#include "JobService.h"
#include "RecordingActionUtils.h"

Job JobService::createJob(
    int recordingId,
    RecordingActionType actionType)
{
    Job job;

    job.recordingId = recordingId;
    job.jobType = toString(actionType);
    job.status = "PENDING";
    job.priority = 0;
    job.message = "";

    return job;
}

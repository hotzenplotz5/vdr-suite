#include "Database.h"

#include "ActionService.h"
#include "JobRepository.h"
#include "JobService.h"
#include "RecordingWorkflowService.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        return 1;
    }

    ActionService actionService;
    JobService jobService;
    JobRepository jobRepository(db);

    RecordingWorkflowService workflow(
        actionService,
        jobService,
        jobRepository);

    auto job =
        workflow.createActionJob(
            1,
            RecordingActionType::Shrink);

    std::cout << "Recording: " << job.recordingId << std::endl;
    std::cout << "Type: " << job.jobType << std::endl;
    std::cout << "Status: " << job.status << std::endl;

    auto jobs = jobRepository.getAllJobs();

    std::cout << "Jobs in DB: " << jobs.size() << std::endl;

    return 0;
}

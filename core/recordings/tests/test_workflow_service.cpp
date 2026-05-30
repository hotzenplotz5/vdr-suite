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

    workflow.createCheckJob(1);
    workflow.createShrinkJob(1);
    workflow.createRepairJob(1);
    workflow.createCutJob(1);
    workflow.createPes2TsJob(1);
    workflow.createRenameJob(1);

    auto jobs =
        jobRepository.getAllJobs();

    for (const auto& job : jobs)
    {
        std::cout
            << job.id << " | "
            << job.jobType << " | "
            << job.status
            << std::endl;
    }

    return 0;
}

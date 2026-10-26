#include "Database.h"
#include "TestDatabasePath.h"
#include "JobRepository.h"
#include "JobService.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open(vdrSuiteTestDatabasePath()))
    {
        return 1;
    }

    JobService jobService;
    JobRepository repository(db);

    auto job =
        jobService.createJob(
            1,
            RecordingActionType::Shrink);

    repository.insertJob(job);

    repository.updateJobStatus(
        1,
        "RUNNING");

    repository.updateJobStatus(
        1,
        "DONE");

    auto jobs =
        repository.getAllJobs();

    for (const auto& item : jobs)
    {
        std::cout
            << item.id
            << " | "
            << item.jobType
            << " | "
            << item.status
            << std::endl;
    }

    return 0;
}

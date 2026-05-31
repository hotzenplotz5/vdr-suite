#include "Database.h"

#include "JobRepository.h"
#include "JobService.h"
#include "WorkerSimulator.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        return 1;
    }

    JobRepository repository(db);
    JobService jobService;

    auto job =
        jobService.createJob(
            1,
            RecordingActionType::Shrink);

    if (!repository.insertJob(job))
    {
        std::cerr << "insert failed" << std::endl;
        return 1;
    }

    WorkerSimulator worker(repository);

    if (!worker.executeJob(3))
    {
        std::cerr << "worker failed" << std::endl;
        return 1;
    }

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

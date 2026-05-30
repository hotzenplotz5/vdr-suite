#include "Database.h"
#include "JobRepository.h"
#include "JobService.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        return 1;
    }

    JobService jobService;
    JobRepository repository(db);

    auto job =
        jobService.createJob(
            1,
            RecordingActionType::Shrink);

    if (!repository.insertJob(job))
    {
        std::cerr << "insert failed\n";
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

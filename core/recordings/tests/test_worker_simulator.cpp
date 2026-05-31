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
    WorkerSimulator worker(repository);

    worker.processNextJob();

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

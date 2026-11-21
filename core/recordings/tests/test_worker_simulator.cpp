#include "Database.h"
#include "TestDatabasePath.h"

#include "JobRepository.h"
#include "JobService.h"
#include "WorkerSimulator.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open(vdrSuiteTestDatabasePath()))
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

#pragma once

#include "Job.h"

#include <vector>

class Database;

class JobRepository
{
public:
    explicit JobRepository(Database& database);

    bool insertJob(const Job& job);

    std::vector<Job> getAllJobs();

    bool updateJobStatus(
        int jobId,
        const std::string& status);

    Job getNextPendingJob();

private:
    Database& database_;
};

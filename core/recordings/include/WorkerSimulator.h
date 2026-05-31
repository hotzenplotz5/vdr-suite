#pragma once

class JobRepository;

class WorkerSimulator
{
public:
    explicit WorkerSimulator(
        JobRepository& repository);

    bool executeJob(int jobId);

private:
    JobRepository& repository_;
};

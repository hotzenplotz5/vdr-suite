#pragma once

class JobRepository;

class WorkerSimulator
{
public:
    explicit WorkerSimulator(
        JobRepository& repository);

    bool executeJob(int jobId);

    bool processNextJob();

private:
    JobRepository& repository_;
};

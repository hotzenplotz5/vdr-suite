#include "WorkerSimulator.h"
#include "JobRepository.h"

WorkerSimulator::WorkerSimulator(
    JobRepository& repository)
    : repository_(repository)
{
}

bool WorkerSimulator::executeJob(int jobId)
{
    if (!repository_.updateJobStatus(
            jobId,
            "RUNNING"))
    {
        return false;
    }

    if (!repository_.updateJobStatus(
            jobId,
            "DONE"))
    {
        return false;
    }

    return true;
}

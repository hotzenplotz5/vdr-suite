#include "JobService.h"

#include <iostream>

int main()
{
    JobService service;

    auto job =
        service.createJob(
            1,
            RecordingActionType::Shrink);

    std::cout
        << "Recording: "
        << job.recordingId
        << std::endl;

    std::cout
        << "Job Type: "
        << job.jobType
        << std::endl;

    std::cout
        << "Status: "
        << job.status
        << std::endl;

    return 0;
}

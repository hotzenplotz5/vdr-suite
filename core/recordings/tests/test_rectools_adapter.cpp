#include "JobService.h"
#include "RectoolsAdapter.h"

#include <iostream>

int main()
{
    JobService jobService;
    RectoolsAdapter adapter;

    auto job =
        jobService.createJob(
            1,
            RecordingActionType::Shrink);

    if (!adapter.execute(job))
    {
        std::cerr << "adapter failed" << std::endl;
        return 1;
    }

    std::cout
        << "RectoolsAdapter accepted job: "
        << job.jobType
        << std::endl;

    return 0;
}

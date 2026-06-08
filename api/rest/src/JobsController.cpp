#include "JobsController.h"

#include "JobRepository.h"

#include <sstream>

JobsController::JobsController(
    JobRepository& jobRepository)
    : jobRepository_(jobRepository)
{
}

ApiResponse JobsController::getJobs()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";

    const auto jobs =
        jobRepository_.getAllJobs();

    std::ostringstream json;

    json << "{";
    json << "\"jobs\":[";

    for (std::size_t i = 0; i < jobs.size(); ++i)
    {
        const auto& job = jobs[i];

        json << "{";
        json << "\"id\":" << job.id << ",";
        json << "\"recordingId\":" << job.recordingId << ",";
        json << "\"jobType\":\"" << job.jobType << "\",";
        json << "\"status\":\"" << job.status << "\"";
        json << "}";

        if (i + 1 < jobs.size())
        {
            json << ",";
        }
    }

    json << "]";
    json << "}";

    response.body =
        json.str();

    return response;
}

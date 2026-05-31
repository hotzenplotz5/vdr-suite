#include "JobsController.h"

#include "Database.h"
#include "JobRepository.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        std::cerr << "database open failed" << std::endl;
        return 1;
    }

    JobRepository jobRepository(db);

    JobsController controller(jobRepository);

    ApiResponse response =
        controller.getJobs();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");

    assert(response.body.find("\"jobs\"") != std::string::npos);
    assert(response.body.find("\"id\":1") != std::string::npos);
    assert(response.body.find("\"jobType\":\"SHRINK\"") != std::string::npos);
    assert(response.body.find("\"status\":\"PENDING\"") != std::string::npos);

    db.close();

    std::cout
        << response.body
        << std::endl;

    std::cout
        << "test_jobs_controller passed"
        << std::endl;

    return 0;
}

#pragma once

#include "DashboardController.h"

class JobRepository;

class JobsController
{
public:
    explicit JobsController(
        JobRepository& jobRepository);

    ApiResponse getJobs();

private:
    JobRepository& jobRepository_;
};

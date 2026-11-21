#pragma once

#include "DashboardSummary.h"

class JobRepository;

class JobDashboardService
{
public:
    explicit JobDashboardService(
        JobRepository& repository);

    DashboardSummary getSummary();

private:
    JobRepository& repository_;
};

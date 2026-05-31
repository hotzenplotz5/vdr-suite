#pragma once

#include "DashboardController.h"

#include <string>

class JobsController;

class ApiRouter
{
public:
    ApiRouter(
        DashboardController& dashboardController,
        JobsController& jobsController);

    ApiResponse handleGet(
        const std::string& path);

private:
    DashboardController& dashboardController_;
    JobsController& jobsController_;
};

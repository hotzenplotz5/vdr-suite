#pragma once

#include "DashboardController.h"

#include <string>

class ApiRouter
{
public:
    explicit ApiRouter(
        DashboardController& dashboardController);

    ApiResponse handleGet(
        const std::string& path);

private:
    DashboardController& dashboardController_;
};

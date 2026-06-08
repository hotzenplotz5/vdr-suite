#pragma once

#include <string>

class DashboardFacade;
class DashboardJsonSerializer;

struct ApiResponse
{
    int statusCode = 200;
    std::string contentType = "application/json";
    std::string body;
};

class DashboardController
{
public:
    DashboardController(
        DashboardFacade& dashboardFacade,
        DashboardJsonSerializer& jsonSerializer);

    ApiResponse getDashboard();

private:
    DashboardFacade& dashboardFacade_;
    DashboardJsonSerializer& jsonSerializer_;
};

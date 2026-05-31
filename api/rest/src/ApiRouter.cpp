#include "ApiRouter.h"

ApiRouter::ApiRouter(
    DashboardController& dashboardController)
    : dashboardController_(dashboardController)
{
}

ApiResponse ApiRouter::handleGet(
    const std::string& path)
{
    if (path == "/api/dashboard")
    {
        return dashboardController_.getDashboard();
    }

    ApiResponse response;

    response.statusCode = 404;
    response.contentType = "application/json";
    response.body = "{\"error\":\"not found\"}";

    return response;
}

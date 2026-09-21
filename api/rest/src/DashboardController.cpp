#include "DashboardController.h"

#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"

DashboardController::DashboardController(
    DashboardFacade& dashboardFacade,
    DashboardJsonSerializer& jsonSerializer)
    : dashboardFacade_(dashboardFacade),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse DashboardController::getDashboard()
{
    ApiResponse response;

    const auto overview =
        dashboardFacade_.getOverview();

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(overview);

    return response;
}

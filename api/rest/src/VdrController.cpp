#include "VdrController.h"

#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"

VdrController::VdrController(
    VdrOverviewService& overviewService,
    VdrOverviewJsonSerializer& jsonSerializer)
    : overviewService_(overviewService),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse VdrController::getOverview()
{
    ApiResponse response;

    const auto overview =
        overviewService_.getOverview();

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(overview);

    return response;
}

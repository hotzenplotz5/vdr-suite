#include "CapabilityController.h"

#include "CapabilityReportJsonSerializer.h"
#include "CapabilityReportService.h"

CapabilityController::CapabilityController(
    CapabilityReportService& reportService,
    CapabilityReportJsonSerializer& jsonSerializer)
    : reportService_(reportService),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse CapabilityController::getCapabilities()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(
            reportService_.getReport());

    return response;
}

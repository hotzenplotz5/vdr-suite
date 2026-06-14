#include "CapabilityController.h"

#include "CapabilityReport.h"
#include "CapabilityReportJsonSerializer.h"

CapabilityController::CapabilityController(
    CapabilityReport& report,
    CapabilityReportJsonSerializer& jsonSerializer)
    : report_(report),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse CapabilityController::getCapabilities()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = jsonSerializer_.serialize(report_);

    return response;
}

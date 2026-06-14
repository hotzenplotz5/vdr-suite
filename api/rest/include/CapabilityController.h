#pragma once

#include "DashboardController.h"

class CapabilityReport;
class CapabilityReportJsonSerializer;

class CapabilityController
{
public:
    CapabilityController(
        CapabilityReport& report,
        CapabilityReportJsonSerializer& jsonSerializer);

    ApiResponse getCapabilities();

private:
    CapabilityReport& report_;
    CapabilityReportJsonSerializer& jsonSerializer_;
};

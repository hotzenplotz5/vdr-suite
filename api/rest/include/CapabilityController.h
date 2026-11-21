#pragma once

#include "DashboardController.h"

class CapabilityReportJsonSerializer;
class CapabilityReportService;

class CapabilityController
{
public:
    CapabilityController(
        CapabilityReportService& reportService,
        CapabilityReportJsonSerializer& jsonSerializer);

    ApiResponse getCapabilities();

private:
    CapabilityReportService& reportService_;
    CapabilityReportJsonSerializer& jsonSerializer_;
};

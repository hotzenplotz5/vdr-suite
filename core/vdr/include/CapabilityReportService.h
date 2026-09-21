#pragma once

#include "CapabilityReport.h"
#include "CapabilityReportBuilder.h"
#include "ICapabilityResolver.h"

#include <string>

class CapabilityReportService
{
public:
    CapabilityReportService(
        std::string backendId,
        ICapabilityResolver& resolver,
        CapabilityReportBuilder& reportBuilder);

    CapabilityReport getReport() const;

private:
    std::string backendId_;
    ICapabilityResolver& resolver_;
    CapabilityReportBuilder& reportBuilder_;
};

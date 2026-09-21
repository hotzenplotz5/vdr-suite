#include "CapabilityReportService.h"

#include <utility>

CapabilityReportService::CapabilityReportService(
    std::string backendId,
    ICapabilityResolver& resolver,
    CapabilityReportBuilder& reportBuilder)
    : backendId_(std::move(backendId)),
      resolver_(resolver),
      reportBuilder_(reportBuilder)
{
}

CapabilityReport CapabilityReportService::getReport() const
{
    return reportBuilder_.build(
        backendId_,
        resolver_);
}

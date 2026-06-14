#pragma once

#include "CapabilityReport.h"
#include "CapabilityStateJsonSerializer.h"

#include <string>

class CapabilityReportJsonSerializer
{
public:
    std::string serialize(
        const CapabilityReport& report) const;

private:
    CapabilityStateJsonSerializer stateSerializer_;
};

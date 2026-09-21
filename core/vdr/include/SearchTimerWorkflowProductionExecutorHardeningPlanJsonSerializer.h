#pragma once

#include "SearchTimerWorkflowProductionExecutorHardeningPlan.h"

#include <string>

class SearchTimerWorkflowProductionExecutorHardeningPlanJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerWorkflowProductionExecutorHardeningPlanResult& result) const;
};

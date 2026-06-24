#pragma once

#include "SearchTimerWorkflowExecutionPlan.h"

#include <string>

class SearchTimerWorkflowExecutionPlanJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerWorkflowExecutionPlan& plan) const;
};

#pragma once

#include "SearchTimerWorkflowExecutionPlan.h"
#include "SearchTimerWorkflowRequest.h"

class SearchTimerWorkflowPlanningService
{
public:
    SearchTimerWorkflowExecutionPlan plan(
        const SearchTimerWorkflowRequest& request) const;
};

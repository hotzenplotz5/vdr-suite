#pragma once

#include "SearchTimerWorkflowExecutionPlan.h"
#include "SearchTimerWorkflowExecutionResult.h"

class SearchTimerWorkflowExecutionService
{
public:
    SearchTimerWorkflowExecutionResult executePlan(
        const SearchTimerWorkflowExecutionPlan& plan,
        bool explicitOperatorConfirmation = false) const;
};

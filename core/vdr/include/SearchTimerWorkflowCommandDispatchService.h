#pragma once

#include "SearchTimerWorkflowExecutionPlan.h"
#include "SearchTimerWorkflowExecutionResult.h"

class SearchTimerWorkflowCommandDispatchService
{
public:
    SearchTimerWorkflowExecutionResult dispatchPlan(
        const SearchTimerWorkflowExecutionPlan& plan,
        bool explicitOperatorConfirmation = false) const;
};

#pragma once

#include "SearchTimerWorkflowCommandDispatchOptions.h"
#include "SearchTimerWorkflowExecutionPlan.h"
#include "SearchTimerWorkflowExecutionResult.h"

class SearchTimerWorkflowCommandDispatchService
{
public:
    SearchTimerWorkflowExecutionResult dispatchPlan(
        const SearchTimerWorkflowExecutionPlan& plan,
        bool explicitOperatorConfirmation = false) const;

    SearchTimerWorkflowExecutionResult dispatchPlan(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerWorkflowCommandDispatchOptions& options) const;
};

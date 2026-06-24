#pragma once

#include "SearchTimerCreateResult.h"
#include "SearchTimerDeleteResult.h"
#include "SearchTimerUpdateResult.h"
#include "SearchTimerWorkflowExecutionPlan.h"
#include "SearchTimerWorkflowExecutionResult.h"

class SearchTimerWorkflowExecutorResultMapper
{
public:
    SearchTimerWorkflowExecutionResult mapCreateResult(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerCreateResult& executorResult) const;

    SearchTimerWorkflowExecutionResult mapUpdateResult(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerUpdateResult& executorResult) const;

    SearchTimerWorkflowExecutionResult mapDeleteResult(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerDeleteResult& executorResult) const;
};

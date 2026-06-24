#include "SearchTimerWorkflowPlanningService.h"

#include "SearchTimerWorkflowValidationService.h"

SearchTimerWorkflowExecutionPlan SearchTimerWorkflowPlanningService::plan(
    const SearchTimerWorkflowRequest& request) const
{
    SearchTimerWorkflowValidationService validationService;

    const SearchTimerWorkflowValidationResult validation =
        validationService.validate(request);

    if (!validation.valid)
    {
        return SearchTimerWorkflowExecutionPlan::fromRequest(request);
    }

    return SearchTimerWorkflowExecutionPlan::fromRequest(request);
}

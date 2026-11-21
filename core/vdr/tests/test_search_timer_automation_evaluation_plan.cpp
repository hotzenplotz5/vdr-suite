#include "SearchTimerAutomationEvaluationPlan.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    SearchTimerAutomationEvaluationPlan plan =
        SearchTimerAutomationEvaluationPlan::createReadOnly("default");

    assert(plan.backendId() == "default");
    assert(plan.dryRunOnly());
    assert(!plan.mutationAllowed());
    assert(!plan.scheduledExecutionAllowed());
    assert(plan.requiresExplicitExecutionHandoff());
    assert(plan.candidateLimit() == 50);
    assert(!plan.includeInactiveSearchTimers());
    assert(plan.includeExistingTimers());
    assert(plan.includeRecordings());
    assert(plan.isValid());
    assert(plan.validationErrors().empty());

    plan.setCandidateLimit(25);
    assert(plan.candidateLimit() == 25);

    plan.setCandidateLimit(0);
    assert(plan.candidateLimit() == 1);
    assert(plan.isValid());

    plan.setIncludeInactiveSearchTimers(true);
    plan.setIncludeExistingTimers(false);
    plan.setIncludeRecordings(false);

    assert(plan.includeInactiveSearchTimers());
    assert(!plan.includeExistingTimers());
    assert(!plan.includeRecordings());

    SearchTimerAutomationEvaluationPlan invalidPlan =
        SearchTimerAutomationEvaluationPlan::createReadOnly("");

    assert(!invalidPlan.isValid());

    const std::vector<std::string> errors =
        invalidPlan.validationErrors();

    assert(errors.size() == 1);
    assert(errors.front() == "backend id is required");
    assert(invalidPlan.dryRunOnly());
    assert(!invalidPlan.mutationAllowed());
    assert(!invalidPlan.scheduledExecutionAllowed());

    std::cout
        << "test_search_timer_automation_evaluation_plan passed"
        << std::endl;

    return 0;
}

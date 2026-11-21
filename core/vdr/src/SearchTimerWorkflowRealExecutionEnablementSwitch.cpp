#include "SearchTimerWorkflowRealExecutionEnablementSwitch.h"

SearchTimerWorkflowRealExecutionEnablementSwitchDecision
SearchTimerWorkflowRealExecutionEnablementSwitch::evaluate() const
{
    SearchTimerWorkflowRealExecutionEnablementSwitchDecision decision;

    if (!enabled_)
    {
        decision.enabled = false;
        decision.dispatchStage = "real-execution-enable-switch-disabled";
        decision.message = "production real execution is disabled";
        decision.errors = {"production real execution enable switch is disabled"};
        return decision;
    }

    decision.enabled = true;
    decision.dispatchStage = "real-execution-enable-switch-enabled";
    decision.message = "production real execution enable switch is enabled";
    decision.errors.clear();
    return decision;
}

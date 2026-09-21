#include "SearchTimerWorkflowRealExecutionEnablementSwitch.h"

#include <cassert>
#include <iostream>

int main()
{
    const SearchTimerWorkflowRealExecutionEnablementSwitchDecision disabled =
        SearchTimerWorkflowRealExecutionEnablementSwitch::disabledByDefault().evaluate();

    assert(!disabled.enabled);
    assert(disabled.dispatchStage == "real-execution-enable-switch-disabled");
    assert(disabled.message == "production real execution is disabled");
    assert(!disabled.errors.empty());

    const SearchTimerWorkflowRealExecutionEnablementSwitchDecision enabled =
        SearchTimerWorkflowRealExecutionEnablementSwitch::enabledForProductionConfiguration().evaluate();

    assert(enabled.enabled);
    assert(enabled.dispatchStage == "real-execution-enable-switch-enabled");
    assert(enabled.message == "production real execution enable switch is enabled");
    assert(enabled.errors.empty());

    std::cout
        << "test_search_timer_workflow_real_execution_enablement_switch passed"
        << std::endl;

    return 0;
}

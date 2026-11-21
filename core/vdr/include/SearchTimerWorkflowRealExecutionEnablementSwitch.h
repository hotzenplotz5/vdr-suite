#pragma once

#include <string>
#include <vector>

struct SearchTimerWorkflowRealExecutionEnablementSwitchDecision
{
    bool enabled = false;
    std::string dispatchStage = "real-execution-enable-switch-disabled";
    std::string message = "production real execution is disabled";
    std::vector<std::string> errors;
};

class SearchTimerWorkflowRealExecutionEnablementSwitch
{
public:
    static SearchTimerWorkflowRealExecutionEnablementSwitch disabledByDefault()
    {
        return SearchTimerWorkflowRealExecutionEnablementSwitch(false);
    }

    static SearchTimerWorkflowRealExecutionEnablementSwitch enabledForProductionConfiguration()
    {
        return SearchTimerWorkflowRealExecutionEnablementSwitch(true);
    }

    SearchTimerWorkflowRealExecutionEnablementSwitchDecision evaluate() const;

private:
    explicit SearchTimerWorkflowRealExecutionEnablementSwitch(
        bool enabled)
        : enabled_(enabled)
    {
    }

    bool enabled_ = false;
};

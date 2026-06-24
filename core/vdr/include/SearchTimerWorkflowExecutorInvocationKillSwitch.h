#pragma once

#include "SearchTimerWorkflowGuardedExecutorInvocation.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowExecutorInvocationKillSwitchDecision
{
    bool killSwitchOpen = false;
    bool allowed = false;
    std::string dispatchStage = "executor-invocation-kill-switch-closed";
    std::string message = "executor invocation kill switch is closed";
    std::vector<std::string> errors = {
        "executor invocation kill switch is closed"};
};

class SearchTimerWorkflowExecutorInvocationKillSwitch
{
public:
    static SearchTimerWorkflowExecutorInvocationKillSwitch closed()
    {
        return SearchTimerWorkflowExecutorInvocationKillSwitch(false);
    }

    static SearchTimerWorkflowExecutorInvocationKillSwitch openedForControlledExecution()
    {
        return SearchTimerWorkflowExecutorInvocationKillSwitch(true);
    }

    bool open() const
    {
        return open_;
    }

    SearchTimerWorkflowExecutorInvocationKillSwitchDecision evaluate(
        const SearchTimerWorkflowGuardedExecutorInvocationDecision& guardDecision) const;

private:
    explicit SearchTimerWorkflowExecutorInvocationKillSwitch(
        bool open)
        : open_(open)
    {
    }

    bool open_ = false;
};

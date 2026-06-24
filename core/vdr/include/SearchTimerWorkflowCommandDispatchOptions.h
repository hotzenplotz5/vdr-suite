#pragma once

#include "ISearchTimerCommandExecutor.h"

class SearchTimerWorkflowCommandDispatchOptions
{
public:
    static SearchTimerWorkflowCommandDispatchOptions defaults()
    {
        return SearchTimerWorkflowCommandDispatchOptions();
    }

    static SearchTimerWorkflowCommandDispatchOptions confirmed(
        bool explicitOperatorConfirmation)
    {
        SearchTimerWorkflowCommandDispatchOptions options;
        options.explicitOperatorConfirmation_ =
            explicitOperatorConfirmation;
        return options;
    }

    static SearchTimerWorkflowCommandDispatchOptions confirmedWithExecutorOptIn(
        bool explicitOperatorConfirmation)
    {
        SearchTimerWorkflowCommandDispatchOptions options;
        options.explicitOperatorConfirmation_ =
            explicitOperatorConfirmation;
        options.executorOptInEnabled_ = true;
        return options;
    }

    static SearchTimerWorkflowCommandDispatchOptions confirmedWithExecutorOptInAndExecutor(
        bool explicitOperatorConfirmation,
        ISearchTimerCommandExecutor* executor)
    {
        SearchTimerWorkflowCommandDispatchOptions options;
        options.explicitOperatorConfirmation_ =
            explicitOperatorConfirmation;
        options.executorOptInEnabled_ = true;
        options.commandExecutor_ = executor;
        return options;
    }

    static SearchTimerWorkflowCommandDispatchOptions confirmedWithControlledTestExecutorInvocation(
        bool explicitOperatorConfirmation,
        ISearchTimerCommandExecutor* executor)
    {
        SearchTimerWorkflowCommandDispatchOptions options;
        options.explicitOperatorConfirmation_ =
            explicitOperatorConfirmation;
        options.executorOptInEnabled_ = true;
        options.commandExecutor_ = executor;
        options.controlledTestExecutorInvocationEnabled_ = true;
        return options;
    }

    static SearchTimerWorkflowCommandDispatchOptions confirmedWithProductionRealExecutionEnabled(
        bool explicitOperatorConfirmation,
        ISearchTimerCommandExecutor* executor)
    {
        SearchTimerWorkflowCommandDispatchOptions options;
        options.explicitOperatorConfirmation_ =
            explicitOperatorConfirmation;
        options.executorOptInEnabled_ = true;
        options.commandExecutor_ = executor;
        options.productionRealExecutionEnabled_ = true;
        return options;
    }

    bool explicitOperatorConfirmation() const
    {
        return explicitOperatorConfirmation_;
    }

    bool executorOptInEnabled() const
    {
        return executorOptInEnabled_;
    }

    bool hasCommandExecutor() const
    {
        return commandExecutor_ != nullptr;
    }

    ISearchTimerCommandExecutor* commandExecutor() const
    {
        return commandExecutor_;
    }

    bool controlledTestExecutorInvocationEnabled() const
    {
        return controlledTestExecutorInvocationEnabled_;
    }

    bool productionRealExecutionEnabled() const
    {
        return productionRealExecutionEnabled_;
    }

private:
    bool explicitOperatorConfirmation_ = false;
    bool executorOptInEnabled_ = false;
    ISearchTimerCommandExecutor* commandExecutor_ = nullptr;
    bool controlledTestExecutorInvocationEnabled_ = false;
    bool productionRealExecutionEnabled_ = false;
};

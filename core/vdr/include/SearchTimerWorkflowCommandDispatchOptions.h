#pragma once

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

    bool explicitOperatorConfirmation() const
    {
        return explicitOperatorConfirmation_;
    }

    bool executorOptInEnabled() const
    {
        return executorOptInEnabled_;
    }

private:
    bool explicitOperatorConfirmation_ = false;
    bool executorOptInEnabled_ = false;
};

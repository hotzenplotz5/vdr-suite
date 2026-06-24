#pragma once

#include "ISearchTimerCommandExecutor.h"

#include <string>
#include <vector>

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

    static SearchTimerWorkflowCommandDispatchOptions confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlist(
        bool explicitOperatorConfirmation,
        ISearchTimerCommandExecutor* executor,
        const std::vector<std::string>& allowedBackendIds)
    {
        SearchTimerWorkflowCommandDispatchOptions options;
        options.explicitOperatorConfirmation_ =
            explicitOperatorConfirmation;
        options.executorOptInEnabled_ = true;
        options.commandExecutor_ = executor;
        options.productionRealExecutionEnabled_ = true;
        options.backendWriteAllowlistEnabled_ = true;
        options.backendWriteAllowedBackendIds_ = allowedBackendIds;
        return options;
    }

    static SearchTimerWorkflowCommandDispatchOptions confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlistAndPermission(
        bool explicitOperatorConfirmation,
        ISearchTimerCommandExecutor* executor,
        const std::vector<std::string>& allowedBackendIds,
        const std::vector<std::string>& permittedBackendIds)
    {
        SearchTimerWorkflowCommandDispatchOptions options;
        options.explicitOperatorConfirmation_ =
            explicitOperatorConfirmation;
        options.executorOptInEnabled_ = true;
        options.commandExecutor_ = executor;
        options.productionRealExecutionEnabled_ = true;
        options.backendWriteAllowlistEnabled_ = true;
        options.backendWriteAllowedBackendIds_ = allowedBackendIds;
        options.backendWritePermissionGateEnabled_ = true;
        options.backendWritePermittedBackendIds_ = permittedBackendIds;
        return options;
    }

    static SearchTimerWorkflowCommandDispatchOptions confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlistAndPermissionAndProductionPolicyGate(
        bool explicitOperatorConfirmation,
        ISearchTimerCommandExecutor* executor,
        const std::vector<std::string>& allowedBackendIds,
        const std::vector<std::string>& permittedBackendIds)
    {
        SearchTimerWorkflowCommandDispatchOptions options;
        options.explicitOperatorConfirmation_ =
            explicitOperatorConfirmation;
        options.executorOptInEnabled_ = true;
        options.commandExecutor_ = executor;
        options.productionRealExecutionEnabled_ = true;
        options.backendWriteAllowlistEnabled_ = true;
        options.backendWriteAllowedBackendIds_ = allowedBackendIds;
        options.backendWritePermissionGateEnabled_ = true;
        options.backendWritePermittedBackendIds_ = permittedBackendIds;
        options.productionPolicyGateConfigured_ = true;
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

    bool backendWriteAllowlistEnabled() const
    {
        return backendWriteAllowlistEnabled_;
    }

    const std::vector<std::string>& backendWriteAllowedBackendIds() const
    {
        return backendWriteAllowedBackendIds_;
    }

    bool backendWritePermissionGateEnabled() const
    {
        return backendWritePermissionGateEnabled_;
    }

    const std::vector<std::string>& backendWritePermittedBackendIds() const
    {
        return backendWritePermittedBackendIds_;
    }

    bool productionPolicyGateConfigured() const
    {
        return productionPolicyGateConfigured_;
    }

private:
    bool explicitOperatorConfirmation_ = false;
    bool executorOptInEnabled_ = false;
    ISearchTimerCommandExecutor* commandExecutor_ = nullptr;
    bool controlledTestExecutorInvocationEnabled_ = false;
    bool productionRealExecutionEnabled_ = false;
    bool backendWriteAllowlistEnabled_ = false;
    std::vector<std::string> backendWriteAllowedBackendIds_;
    bool backendWritePermissionGateEnabled_ = false;
    std::vector<std::string> backendWritePermittedBackendIds_;
    bool productionPolicyGateConfigured_ = false;
};

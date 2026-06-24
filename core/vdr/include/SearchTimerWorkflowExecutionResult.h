#pragma once

#include "SearchTimerWorkflowExecutionPlan.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowExecutionResult
{
    bool success = false;
    bool executed = false;
    bool blocked = false;
    bool dryRunOnly = true;
    bool confirmationProvided = false;
    bool requiresExplicitOperatorConfirmation = false;
    bool requiresBackendReadback = false;
    bool commandRequestMapped = false;
    bool realExecutionEnabled = false;
    std::string dispatchStage = "none";
    SearchTimerWorkflowExecutionMode executionMode =
        SearchTimerWorkflowExecutionMode::Prepare;
    SearchTimerWorkflowOperation operation =
        SearchTimerWorkflowOperation::Unknown;
    SearchTimerWorkflowExecutionStep primaryStep =
        SearchTimerWorkflowExecutionStep::None;
    SearchTimerWorkflowExecutionStep followUpStep =
        SearchTimerWorkflowExecutionStep::None;
    std::string backendId;
    std::string backendNativeId;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    static SearchTimerWorkflowExecutionResult blockedResult(
        const SearchTimerWorkflowExecutionPlan& plan,
        const std::string& message,
        const std::vector<std::string>& errors)
    {
        SearchTimerWorkflowExecutionResult result;
        result.success = false;
        result.executed = false;
        result.blocked = true;
        result.dryRunOnly = true;
        result.confirmationProvided = false;
        result.requiresExplicitOperatorConfirmation =
            plan.requiresExplicitOperatorConfirmation();
        result.requiresBackendReadback =
            plan.requiresBackendReadback();
        result.commandRequestMapped = false;
        result.realExecutionEnabled = false;
        result.dispatchStage = "blocked";
        result.executionMode = plan.executionMode();
        result.operation = plan.operation();
        result.primaryStep = plan.primaryStep();
        result.followUpStep = plan.followUpStep();
        result.backendId = plan.backendId();
        result.backendNativeId = plan.backendNativeId();
        result.message = message;
        result.errors = errors;
        return result;
    }

    static SearchTimerWorkflowExecutionResult acceptedSkeleton(
        const SearchTimerWorkflowExecutionPlan& plan,
        bool confirmationProvided,
        const std::string& message,
        const std::vector<std::string>& warnings)
    {
        SearchTimerWorkflowExecutionResult result;
        result.success = true;
        result.executed = false;
        result.blocked = false;
        result.dryRunOnly = true;
        result.confirmationProvided = confirmationProvided;
        result.requiresExplicitOperatorConfirmation =
            plan.requiresExplicitOperatorConfirmation();
        result.requiresBackendReadback =
            plan.requiresBackendReadback();
        result.commandRequestMapped = false;
        result.realExecutionEnabled = false;
        result.dispatchStage = "skeleton-accepted";
        result.executionMode = plan.executionMode();
        result.operation = plan.operation();
        result.primaryStep = plan.primaryStep();
        result.followUpStep = plan.followUpStep();
        result.backendId = plan.backendId();
        result.backendNativeId = plan.backendNativeId();
        result.message = message;
        result.warnings = warnings;
        return result;
    }

    bool hasWarnings() const
    {
        return !warnings.empty();
    }

    bool hasErrors() const
    {
        return !errors.empty();
    }
};

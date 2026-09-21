#pragma once

#include "SearchTimerWorkflowExecutionPlan.h"
#include "SearchTimerWorkflowBackendReadbackVerificationResult.h"

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
    bool realExecutionPolicyAllowed = false;
    bool executorOptInProvided = false;
    bool executorInjected = false;
    bool executorInvocationGuardPassed = false;
    bool executorInvocationAttempted = false;
    bool executorInvocationKillSwitchOpen = false;
    bool executorInvocationKillSwitchPassed = false;
    bool executorResultMapped = false;
    bool executorResultSuccessful = false;
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
    std::vector<std::string> executorInvocationAuditTrail;
    bool backendReadbackVerificationAttached = false;
    SearchTimerWorkflowBackendReadbackVerificationResult backendReadbackVerification;

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
        result.realExecutionPolicyAllowed = false;
        result.executorOptInProvided = false;
        result.executorInjected = false;
        result.executorInvocationGuardPassed = false;
        result.executorInvocationAttempted = false;
        result.executorInvocationKillSwitchOpen = false;
        result.executorInvocationKillSwitchPassed = false;
        result.executorResultMapped = false;
        result.executorResultSuccessful = false;
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
        result.realExecutionPolicyAllowed = false;
        result.executorOptInProvided = false;
        result.executorInjected = false;
        result.executorInvocationGuardPassed = false;
        result.executorInvocationAttempted = false;
        result.executorInvocationKillSwitchOpen = false;
        result.executorInvocationKillSwitchPassed = false;
        result.executorResultMapped = false;
        result.executorResultSuccessful = false;
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

    void attachBackendReadbackVerification(
        const SearchTimerWorkflowBackendReadbackVerificationResult& verification)
    {
        backendReadbackVerification = verification;
        backendReadbackVerificationAttached = true;

        warnings.insert(
            warnings.end(),
            verification.warnings.begin(),
            verification.warnings.end());

        if (requiresBackendReadback && !verification.passed())
        {
            success = false;

            if (verification.errors.empty())
            {
                errors.push_back(
                    "backend readback verification failed");
            }
            else
            {
                errors.insert(
                    errors.end(),
                    verification.errors.begin(),
                    verification.errors.end());
            }
        }
    }

    bool backendReadbackVerified() const
    {
        if (!requiresBackendReadback)
        {
            return true;
        }

        return backendReadbackVerificationAttached
            && backendReadbackVerification.passed();
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

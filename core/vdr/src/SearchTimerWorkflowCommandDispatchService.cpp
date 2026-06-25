#include "SearchTimerWorkflowCommandDispatchService.h"

#include "SearchTimerWorkflowCommandRequestMapper.h"
#include "SearchTimerWorkflowCreateReadbackVerificationService.h"
#include "SearchTimerWorkflowDeleteAbsenceVerificationService.h"
#include "SearchTimerWorkflowUpdateReadbackVerificationService.h"
#include "SearchTimerWorkflowExecutorInvocationKillSwitch.h"
#include "SearchTimerWorkflowExecutorResultMapper.h"
#include "SearchTimerWorkflowGuardedExecutorInvocation.h"
#include "SearchTimerWorkflowRealExecutionPolicy.h"

#include <string>
#include <vector>

namespace
{

std::string commandNameForStep(
    SearchTimerWorkflowExecutionStep step)
{
    if (step == SearchTimerWorkflowExecutionStep::Create)
    {
        return "create";
    }

    if (step == SearchTimerWorkflowExecutionStep::Update)
    {
        return "update";
    }

    if (step == SearchTimerWorkflowExecutionStep::Delete)
    {
        return "delete";
    }

    return "none";
}

SearchTimerWorkflowExecutionResult applyDispatchOptions(
    SearchTimerWorkflowExecutionResult result,
    const SearchTimerWorkflowCommandDispatchOptions& options)
{
    result.executorOptInProvided =
        options.executorOptInEnabled();
    result.executorInjected =
        options.hasCommandExecutor();
    return result;
}

const char* auditBoolText(
    bool value)
{
    return value ? "true" : "false";
}

void appendAuditEntry(
    std::vector<std::string>& auditTrail,
    const std::string& key,
    const std::string& value)
{
    auditTrail.push_back(key + "=" + value);
}

void appendAuditEntry(
    std::vector<std::string>& auditTrail,
    const std::string& key,
    const char* value)
{
    appendAuditEntry(
        auditTrail,
        key,
        std::string(value));
}

void appendAuditEntry(
    std::vector<std::string>& auditTrail,
    const std::string& key,
    bool value)
{
    appendAuditEntry(
        auditTrail,
        key,
        std::string(auditBoolText(value)));
}

std::vector<std::string> buildExecutorInvocationAuditTrail(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerWorkflowCommandDispatchOptions& options,
    const SearchTimerWorkflowRealExecutionPolicyDecision& policyDecision,
    const SearchTimerWorkflowGuardedExecutorInvocationDecision& guardDecision,
    const SearchTimerWorkflowExecutorInvocationKillSwitchDecision& killSwitchDecision)
{
    std::vector<std::string> auditTrail;

    appendAuditEntry(
        auditTrail,
        "executionMode",
        plan.executionMode() == SearchTimerWorkflowExecutionMode::Execute
            ? "execute"
            : "non-execute");
    appendAuditEntry(
        auditTrail,
        "commandRequestMapped",
        true);
    appendAuditEntry(
        auditTrail,
        "explicitOperatorConfirmation",
        options.explicitOperatorConfirmation());
    appendAuditEntry(
        auditTrail,
        "executorOptInProvided",
        options.executorOptInEnabled());
    appendAuditEntry(
        auditTrail,
        "executorInjected",
        options.hasCommandExecutor());
    appendAuditEntry(
        auditTrail,
        "controlledTestExecutorInvocation",
        options.controlledTestExecutorInvocationEnabled());
    appendAuditEntry(
        auditTrail,
        "policyStage",
        policyDecision.dispatchStage);
    appendAuditEntry(
        auditTrail,
        "policyAllowed",
        policyDecision.allowed);
    appendAuditEntry(
        auditTrail,
        "guardStage",
        guardDecision.dispatchStage);
    appendAuditEntry(
        auditTrail,
        "guardPassed",
        guardDecision.guardPassed);
    appendAuditEntry(
        auditTrail,
        "killSwitchStage",
        killSwitchDecision.dispatchStage);
    appendAuditEntry(
        auditTrail,
        "killSwitchOpen",
        killSwitchDecision.killSwitchOpen);
    appendAuditEntry(
        auditTrail,
        "killSwitchPassed",
        killSwitchDecision.allowed);

    return auditTrail;
}


void appendReadbackAuditEntry(
    SearchTimerWorkflowExecutionResult& result,
    bool readbackVerified)
{
    result.executorInvocationAuditTrail.push_back(
        "backendReadbackVerificationAttached=true");
    result.executorInvocationAuditTrail.push_back(
        readbackVerified
            ? "backendReadbackVerified=true"
            : "backendReadbackVerified=false");
}

void attachCreateReadbackVerificationIfRequired(
    SearchTimerWorkflowExecutionResult& result,
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerCreateResult& executorResult,
    const SearchTimerWorkflowCommandDispatchOptions& options)
{
    if (!executorResult.success || !plan.requiresBackendReadback())
    {
        return;
    }

    SearchTimerWorkflowCreateReadbackVerificationService verifier;
    const SearchTimerWorkflowBackendReadbackVerificationResult verification =
        verifier.verify(
            executorResult,
            options.readbackDataSource());

    result.attachBackendReadbackVerification(verification);
    appendReadbackAuditEntry(
        result,
        verification.passed());

    if (!verification.passed())
    {
        result.dispatchStage =
            "backend-readback-verification-failed";
    }
}

void attachUpdateReadbackVerificationIfRequired(
    SearchTimerWorkflowExecutionResult& result,
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerUpdateResult& executorResult,
    const SearchTimerWorkflowCommandDispatchOptions& options)
{
    if (!executorResult.success || !plan.requiresBackendReadback())
    {
        return;
    }

    SearchTimerWorkflowUpdateReadbackVerificationService verifier;
    const SearchTimerWorkflowBackendReadbackVerificationResult verification =
        verifier.verify(
            executorResult,
            options.readbackDataSource());

    result.attachBackendReadbackVerification(verification);
    appendReadbackAuditEntry(
        result,
        verification.passed());

    if (!verification.passed())
    {
        result.dispatchStage =
            "backend-readback-verification-failed";
    }
}

void attachDeleteReadbackVerificationIfRequired(
    SearchTimerWorkflowExecutionResult& result,
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerDeleteResult& executorResult,
    const SearchTimerWorkflowCommandDispatchOptions& options)
{
    if (!executorResult.success || !plan.requiresBackendReadback())
    {
        return;
    }

    SearchTimerWorkflowDeleteAbsenceVerificationService verifier;
    const SearchTimerWorkflowBackendReadbackVerificationResult verification =
        verifier.verify(
            executorResult,
            options.readbackDataSource());

    result.attachBackendReadbackVerification(verification);
    appendReadbackAuditEntry(
        result,
        verification.passed());

    if (!verification.passed())
    {
        result.dispatchStage =
            "backend-readback-verification-failed";
    }
}

} // namespace

SearchTimerWorkflowExecutionResult
SearchTimerWorkflowCommandDispatchService::dispatchPlan(
    const SearchTimerWorkflowExecutionPlan& plan,
    bool explicitOperatorConfirmation) const
{
    return dispatchPlan(
        plan,
        SearchTimerWorkflowCommandDispatchOptions::confirmed(
            explicitOperatorConfirmation));
}

SearchTimerWorkflowExecutionResult
SearchTimerWorkflowCommandDispatchService::dispatchPlan(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerWorkflowCommandDispatchOptions& options) const
{
    if (!plan.valid() || !plan.hasExecutionWork())
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::blockedResult(
                plan,
                "workflow plan is not executable",
                {"workflow plan is not executable"});
        result.dispatchStage = "validation-blocked";
        return applyDispatchOptions(result, options);
    }

    if (plan.requiresExplicitOperatorConfirmation() &&
        !options.explicitOperatorConfirmation())
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::blockedResult(
                plan,
                "explicit operator confirmation is required",
                {"explicit operator confirmation is required"});
        result.dispatchStage = "confirmation-required";
        return applyDispatchOptions(result, options);
    }

    if (!plan.writeOperation())
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::acceptedSkeleton(
                plan,
                options.explicitOperatorConfirmation(),
                "read-only workflow plan has no command dispatch work",
                {"no command request is required for read-only workflow step"});
        result.dispatchStage = "read-only-no-dispatch";
        return applyDispatchOptions(result, options);
    }

    if (plan.executionMode() == SearchTimerWorkflowExecutionMode::DryRun)
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::acceptedSkeleton(
                plan,
                options.explicitOperatorConfirmation(),
                "write workflow accepted by dry-run execution mode",
                {"execution mode dryRun does not map command requests"});
        result.commandRequestMapped = false;
        result.realExecutionEnabled = false;
        result.dispatchStage = "dry-run";
        return applyDispatchOptions(result, options);
    }

    SearchTimerWorkflowCommandRequestMapper mapper;
    std::string commandName =
        commandNameForStep(plan.primaryStep());

    if (mapper.canBuildCreateRequest(plan))
    {
        const SearchTimerCreateRequest request =
            mapper.buildCreateRequest(plan);
        (void)request;
    }
    else if (mapper.canBuildUpdateRequest(plan))
    {
        const SearchTimerUpdateRequest request =
            mapper.buildUpdateRequest(plan);
        (void)request;
    }
    else if (mapper.canBuildDeleteRequest(plan))
    {
        const SearchTimerDeleteRequest request =
            mapper.buildDeleteRequest(plan);
        (void)request;
    }
    else
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::blockedResult(
                plan,
                "workflow plan cannot be mapped to a command request",
                {"workflow plan cannot be mapped to a command request"});
        result.dispatchStage = "command-request-mapping-failed";
        return applyDispatchOptions(result, options);
    }

    if (plan.executionMode() == SearchTimerWorkflowExecutionMode::Execute &&
        !options.executorOptInEnabled())
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::blockedResult(
                plan,
                "real execution mode requires executor opt-in",
                {"real execution mode requires executor opt-in"});
        result.commandRequestMapped = true;
        result.realExecutionEnabled = false;
        result.dispatchStage = "executor-opt-in-required";
        return applyDispatchOptions(result, options);
    }

    if (plan.executionMode() == SearchTimerWorkflowExecutionMode::Execute)
    {
        SearchTimerWorkflowRealExecutionPolicy policy;
        const SearchTimerWorkflowRealExecutionPolicyDecision decision =
            policy.evaluate(
                plan,
                options);

        SearchTimerWorkflowGuardedExecutorInvocation invocationGuard;
        const SearchTimerWorkflowGuardedExecutorInvocationDecision
            invocationDecision =
                invocationGuard.evaluate(
                    plan,
                    options,
                    decision,
                    true);

        SearchTimerWorkflowExecutorInvocationKillSwitch killSwitch =
            options.controlledTestExecutorInvocationEnabled()
                ? SearchTimerWorkflowExecutorInvocationKillSwitch::openedForControlledExecution()
                : SearchTimerWorkflowExecutorInvocationKillSwitch::closed();
        const SearchTimerWorkflowExecutorInvocationKillSwitchDecision
            killSwitchDecision =
                killSwitch.evaluate(invocationDecision);

        std::vector<std::string> auditTrail =
            buildExecutorInvocationAuditTrail(
                plan,
                options,
                decision,
                invocationDecision,
                killSwitchDecision);

        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::blockedResult(
                plan,
                killSwitchDecision.message,
                killSwitchDecision.errors);
        result.commandRequestMapped = true;
        result.realExecutionPolicyAllowed = decision.allowed;
        result.realExecutionEnabled = false;
        result.executorInvocationGuardPassed =
            invocationDecision.guardPassed;
        result.executorInvocationAttempted =
            invocationDecision.invocationAttempted;
        result.executorInvocationKillSwitchOpen =
            killSwitchDecision.killSwitchOpen;
        result.executorInvocationKillSwitchPassed =
            killSwitchDecision.allowed;
        result.dispatchStage = killSwitchDecision.dispatchStage;
        auditTrail.push_back("executorInvocationAttempted=false");
        auditTrail.push_back("executorResultMapped=false");
        result.executorInvocationAuditTrail = auditTrail;

        if (killSwitchDecision.allowed)
        {
            SearchTimerWorkflowExecutorResultMapper resultMapper;

            if (mapper.canBuildCreateRequest(plan))
            {
                SearchTimerCreateResult executorResult =
                    options.commandExecutor()->create(
                        mapper.buildCreateRequest(plan));
                SearchTimerWorkflowExecutionResult mappedResult =
                    resultMapper.mapCreateResult(
                        plan,
                        executorResult);
                attachCreateReadbackVerificationIfRequired(
                    mappedResult,
                    plan,
                    executorResult,
                    options);
                auditTrail.push_back("executorInvocationAttempted=true");
                auditTrail.push_back("executorResultMapped=true");
                auditTrail.push_back(
                    mappedResult.executorResultSuccessful
                        ? "executorResultSuccessful=true"
                        : "executorResultSuccessful=false");
                auditTrail.push_back("finalDispatchStage=" + mappedResult.dispatchStage);
                mappedResult.executorInvocationAuditTrail = auditTrail;
                return applyDispatchOptions(
                    mappedResult,
                    options);
            }

            if (mapper.canBuildUpdateRequest(plan))
            {
                SearchTimerUpdateResult executorResult =
                    options.commandExecutor()->update(
                        mapper.buildUpdateRequest(plan));
                SearchTimerWorkflowExecutionResult mappedResult =
                    resultMapper.mapUpdateResult(
                        plan,
                        executorResult);
                attachUpdateReadbackVerificationIfRequired(
                    mappedResult,
                    plan,
                    executorResult,
                    options);
                auditTrail.push_back("executorInvocationAttempted=true");
                auditTrail.push_back("executorResultMapped=true");
                auditTrail.push_back(
                    mappedResult.executorResultSuccessful
                        ? "executorResultSuccessful=true"
                        : "executorResultSuccessful=false");
                auditTrail.push_back("finalDispatchStage=" + mappedResult.dispatchStage);
                mappedResult.executorInvocationAuditTrail = auditTrail;
                return applyDispatchOptions(
                    mappedResult,
                    options);
            }

            if (mapper.canBuildDeleteRequest(plan))
            {
                SearchTimerDeleteResult executorResult =
                    options.commandExecutor()->remove(
                        mapper.buildDeleteRequest(plan));
                SearchTimerWorkflowExecutionResult mappedResult =
                    resultMapper.mapDeleteResult(
                        plan,
                        executorResult);
                attachDeleteReadbackVerificationIfRequired(
                    mappedResult,
                    plan,
                    executorResult,
                    options);
                auditTrail.push_back("executorInvocationAttempted=true");
                auditTrail.push_back("executorResultMapped=true");
                auditTrail.push_back(
                    mappedResult.executorResultSuccessful
                        ? "executorResultSuccessful=true"
                        : "executorResultSuccessful=false");
                auditTrail.push_back("finalDispatchStage=" + mappedResult.dispatchStage);
                mappedResult.executorInvocationAuditTrail = auditTrail;
                return applyDispatchOptions(
                    mappedResult,
                    options);
            }
        }

        return applyDispatchOptions(result, options);
    }

    std::vector<std::string> warnings;
    warnings.push_back(
        "command request mapped by dispatch skeleton");
    warnings.push_back(
        "backend command dispatch is not enabled in this skeleton");

    if (plan.requiresBackendReadback())
    {
        warnings.push_back(
            "backend readback will be required after real execution");
    }

    SearchTimerWorkflowExecutionResult result =
        SearchTimerWorkflowExecutionResult::acceptedSkeleton(
            plan,
            options.explicitOperatorConfirmation(),
            commandName + " command request accepted by dispatch skeleton",
            warnings);
    result.commandRequestMapped = true;
    result.realExecutionEnabled = false;
    result.dispatchStage = "command-request-mapped";
    return applyDispatchOptions(result, options);
}

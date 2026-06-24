#include "SearchTimerWorkflowCommandDispatchService.h"

#include "SearchTimerWorkflowCommandRequestMapper.h"
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

        if (killSwitchDecision.allowed)
        {
            SearchTimerWorkflowExecutorResultMapper resultMapper;

            if (mapper.canBuildCreateRequest(plan))
            {
                SearchTimerCreateResult executorResult =
                    options.commandExecutor()->create(
                        mapper.buildCreateRequest(plan));
                return applyDispatchOptions(
                    resultMapper.mapCreateResult(
                        plan,
                        executorResult),
                    options);
            }

            if (mapper.canBuildUpdateRequest(plan))
            {
                SearchTimerUpdateResult executorResult =
                    options.commandExecutor()->update(
                        mapper.buildUpdateRequest(plan));
                return applyDispatchOptions(
                    resultMapper.mapUpdateResult(
                        plan,
                        executorResult),
                    options);
            }

            if (mapper.canBuildDeleteRequest(plan))
            {
                SearchTimerDeleteResult executorResult =
                    options.commandExecutor()->remove(
                        mapper.buildDeleteRequest(plan));
                return applyDispatchOptions(
                    resultMapper.mapDeleteResult(
                        plan,
                        executorResult),
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

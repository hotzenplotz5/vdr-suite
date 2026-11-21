#include "SearchTimerWorkflowProductionPolicyGate.h"

SearchTimerWorkflowProductionPolicyGateDecision
SearchTimerWorkflowProductionPolicyGate::evaluate(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerWorkflowCommandDispatchOptions& options) const
{
    if (plan.executionMode() != SearchTimerWorkflowExecutionMode::Execute)
    {
        SearchTimerWorkflowProductionPolicyGateDecision decision;
        decision.allowed = true;
        decision.configured = true;
        decision.dispatchStage = "production-policy-gate-not-required";
        decision.message = "production policy gate is not required for non-execute mode";
        decision.errors.clear();
        return decision;
    }

    if (!plan.writeOperation())
    {
        SearchTimerWorkflowProductionPolicyGateDecision decision;
        decision.allowed = true;
        decision.configured = true;
        decision.dispatchStage = "production-policy-gate-not-required";
        decision.message = "production policy gate is not required for read-only workflow";
        decision.errors.clear();
        return decision;
    }

    if (!options.productionPolicyGateConfigured())
    {
        SearchTimerWorkflowProductionPolicyGateDecision decision;
        decision.allowed = false;
        decision.configured = false;
        decision.dispatchStage = "production-policy-gate-required";
        decision.message = "production policy gate is required";
        decision.errors = {"production policy gate is required"};
        return decision;
    }

    SearchTimerWorkflowProductionPolicyGateDecision decision;
    decision.allowed = false;
    decision.configured = true;
    decision.dispatchStage = "production-policy-gate-closed";
    decision.message = "production policy gate is closed for real backend mutation";
    decision.errors = {"production policy gate is closed for real backend mutation"};
    return decision;
}

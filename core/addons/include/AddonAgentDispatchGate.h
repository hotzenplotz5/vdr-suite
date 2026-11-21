#pragma once

#include "AddonProviderApproval.h"
#include "BackendAgentCommand.h"
#include "MutationOperation.h"

namespace vdrsuite::addons {

// Read-only pre-dispatch guard for future integration with the EXISTING
// BackendAgentCommand and MutationOperation owners. No new scheduler/worker,
// no handler execution, no command reservation and no entitlement issuance.
enum class DispatchGateReason {
    unsupported, invalidOperation, staleGeneration, invalidAssignment,
    mismatchedOperation, approvalUnavailable
};

struct DispatchGateDecision {
    bool permitted = false;
    DispatchGateReason reason = DispatchGateReason::unsupported;
};

inline DispatchGateDecision previewMediaImportDispatch(
    const vdrsuite::operations::MutationOperation& operation,
    const BackendAgentCommandAssignment& assignment,
    const ProviderApproval& approval,
    const ProviderBinding& provider,
    std::int64_t nowSeconds)
{
    // Until a separately reviewed media import Agent executor exists, no
    // command type can be registered or dispatched by this preview.
    if (operation.backendId.empty() || operation.operationId.empty() ||
        operation.backendGeneration == 0)
        return {false, DispatchGateReason::invalidOperation};
    if (!assignment.present || assignment.operationId.empty() ||
        assignment.backendId.empty() || assignment.commandId.empty() ||
        assignment.commandType.empty() || nowSeconds <= 0 ||
        operation.deadline <= nowSeconds ||
        assignment.deadline <= nowSeconds ||
        assignment.deadline > operation.deadline)
        return {false, DispatchGateReason::invalidAssignment};
    if (operation.backendGeneration != assignment.backendGeneration ||
        operation.backendGeneration != approval.backendGeneration ||
        operation.backendGeneration != provider.observedGeneration)
        return {false, DispatchGateReason::staleGeneration};
    if (operation.operationId != assignment.operationId ||
        operation.backendId != assignment.backendId ||
        assignment.backendId != approval.backendId ||
        assignment.backendId != provider.backendId)
        return {false, DispatchGateReason::mismatchedOperation};
    if (compareApprovedProvider(approval, provider).reason !=
        ApprovalReason::disabledProvider)
        return {false, DispatchGateReason::approvalUnavailable};
    return {false, DispatchGateReason::unsupported};
}

} // namespace vdrsuite::addons

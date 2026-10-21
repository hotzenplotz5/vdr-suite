#pragma once

#include "MediaToolsAgentRequest.h"
#include "BackendAgentCommand.h"
#include "MutationOperation.h"

namespace vdrsuite::mediatools {

// Media Tools never owns command reservation, scheduling or actor grants.
// This strictly observational adapter reads the *canonical* Core/Agent
// objects and cannot turn them into executable authority.
inline ImportEnvelopeReason inspectCanonicalImportAssignment(
    const vdrsuite::operations::MutationOperation& operation,
    const BackendAgentCommandAssignment& assignment,
    std::int64_t now)
{
    const AgentImportEnvelope envelope{
        "rectools", "vdr-suite-addon-media-tools",
        assignment.commandType, assignment.operationId,
        assignment.commandId, assignment.backendId,
        operation.backendGeneration, assignment.backendGeneration,
        operation.deadline, assignment.deadline
    };
    if (!assignment.present || assignment.operationId != operation.operationId ||
        assignment.backendId != operation.backendId ||
        assignment.jobId.empty() || assignment.attemptId.empty() ||
        assignment.claimEpoch == 0 ||
        assignment.agentId.empty() || assignment.agentInstanceId.empty() ||
        operation.requestFingerprint.empty() ||
        assignment.requestFingerprint != operation.requestFingerprint ||
        assignment.payloadVersion == 0 ||
        assignment.verificationPolicy != "reconciliation_required" ||
        operation.verificationPolicy !=
            vdrsuite::operations::MutationOperationVerificationPolicy::
                reconciliationRequired ||
        operation.state != vdrsuite::operations::MutationOperationState::
            dispatching)
        return ImportEnvelopeReason::missingOperation;
    return inspectImportEnvelope(envelope, now);
}

} // namespace vdrsuite::mediatools

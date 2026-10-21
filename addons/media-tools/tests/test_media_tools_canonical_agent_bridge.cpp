#include "MediaToolsCanonicalAgentBridge.h"
#include <cassert>

int main()
{
    using namespace vdrsuite::mediatools;
    vdrsuite::operations::MutationOperation op;
    op.operationId = "operation-a";
    op.backendId = "backend-a";
    op.backendGeneration = 7;
    op.deadline = 200;
    op.state = vdrsuite::operations::MutationOperationState::dispatching;
    op.verificationPolicy = vdrsuite::operations::MutationOperationVerificationPolicy::reconciliationRequired;
    BackendAgentCommandAssignment command;
    command.present = true;
    command.operationId = "operation-a";
    command.jobId = "job-a";
    command.attemptId = "attempt-a";
    command.claimEpoch = 1;
    command.commandId = "command-a";
    command.backendId = "backend-a";
    command.backendGeneration = 7;
    command.commandType = "addon.media.import";
    command.deadline = 150;
    command.requestFingerprint = "fixed-fingerprint";
    command.payloadVersion = 1;
    command.verificationPolicy = "reconciliation_required";
    auto reason = [&] { return inspectCanonicalImportAssignment(op, command, 100); };
    assert(reason() == ImportEnvelopeReason::candidateOnly);
    command.operationId = "wrong";
    assert(reason() == ImportEnvelopeReason::missingOperation);
    command.operationId = op.operationId;
    command.backendGeneration = 8;
    assert(reason() == ImportEnvelopeReason::staleGeneration);
    command.backendGeneration = 7;
    command.commandType = "addon.media.shrink";
    assert(reason() == ImportEnvelopeReason::invalidIdentity);
    command.commandType = "addon.media.import";
    command.claimEpoch = 0;
    assert(reason() == ImportEnvelopeReason::missingOperation);
    command.claimEpoch = 1;
    assert(reason() == ImportEnvelopeReason::candidateOnly);
    op.state = vdrsuite::operations::MutationOperationState::succeeded;
    assert(reason() == ImportEnvelopeReason::missingOperation);
    op.state = vdrsuite::operations::MutationOperationState::dispatching;
    command.verificationPolicy = "none";
    assert(reason() == ImportEnvelopeReason::missingOperation);
    command.verificationPolicy = "reconciliation_required";
    command.payloadVersion = 0;
    assert(reason() == ImportEnvelopeReason::missingOperation);
    command.payloadVersion = 1;
    command.requestFingerprint.clear();
    assert(reason() == ImportEnvelopeReason::missingOperation);
}

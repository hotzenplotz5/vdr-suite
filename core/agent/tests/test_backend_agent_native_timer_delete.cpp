#include "BackendAgentNativeTimerDelete.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::agent;

namespace
{
BackendAgentLocalProviderSelection selection()
{
    BackendAgentLocalProviderSelection value;
    value.backendId = "backend:a";
    value.authorityDomain = kBackendAgentNativeTimerDeleteAuthorityDomain;
    value.providerId = kBackendAgentNativeTimerDeleteProviderId;
    value.providerKind = kBackendAgentNativeTimerDeleteProviderKind;
    value.ownershipGeneration = 3;
    value.providerInstanceEpoch = "plugin-epoch:9";
    value.providerGeneration = 1;
    value.capabilityRevision = 1;
    value.requiredCapability = kBackendAgentNativeTimerDeleteCapability;
    return value;
}

BackendAgentNativeTimerDeleteCommand command()
{
    BackendAgentNativeTimerDeleteCommand value;
    value.commandId = "command:1";
    value.requestFingerprint = "sha256:request-1";
    value.operationId = "operation:delete:1";
    value.operationRevision = "2";
    value.nativeTimerBindingId = "binding:1";
    value.expectedBindingRevision = "7";
    value.expectedNativeTimerFingerprint = "aabbccdd";
    value.timerAssignmentId = "assignment:1";
    value.backendNativeTimerId = "timer:17";
    value.jobId = "job:1";
    value.attemptId = "attempt:1";
    value.claimEpoch = 4;
    value.backendId = "backend:a";
    value.agentId = "agent:1";
    value.agentInstanceId = "agent-instance:1";
    value.backendGeneration = 8;
    value.controlPlaneClaimedAt = 2000;
    value.localProviderSelection = selection();
    return value;
}

BackendAgentNativeTimerDeleteEvidence evidence(
    BackendAgentNativeTimerDeleteOutcomeCategory outcome)
{
    const auto source = command();
    BackendAgentNativeTimerDeleteEvidence value;
    value.commandId = source.commandId;
    value.requestFingerprint = source.requestFingerprint;
    value.operationId = source.operationId;
    value.operationRevision = source.operationRevision;
    value.jobId = source.jobId;
    value.attemptId = source.attemptId;
    value.claimEpoch = source.claimEpoch;
    value.backendId = source.backendId;
    value.agentId = source.agentId;
    value.agentInstanceId = source.agentInstanceId;
    value.backendGeneration = source.backendGeneration;
    value.providerInstanceEpoch = source.localProviderSelection.providerInstanceEpoch;
    value.localStartingPersistedAt = 2050;
    value.outcome = outcome;
    value.dispatchStartedAt =
        outcome == BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect
            ? 0
            : 2060;
    value.completedAt = 2100;
    value.evidenceReference = "suitebridge-receipt:delete-1";
    return value;
}
}

int main()
{
    std::string reason;
    const auto valid = command();
    assert(backendAgentNativeTimerDeleteValidCommand(valid, reason));
    assert(reason.empty());

    auto noNativeFingerprint = valid;
    noNativeFingerprint.expectedNativeTimerFingerprint.clear();
    assert(!backendAgentNativeTimerDeleteValidCommand(noNativeFingerprint, reason));
    assert(reason == "invalid-command-identity");

    auto invalidNativeFingerprint = valid;
    invalidNativeFingerprint.expectedNativeTimerFingerprint = "not-hex";
    assert(!backendAgentNativeTimerDeleteValidCommand(
        invalidNativeFingerprint, reason));
    assert(reason == "invalid-command-identity");

    auto wrongCapability = valid;
    wrongCapability.localProviderSelection.requiredCapability = "vdr.native.probe";
    assert(!backendAgentNativeTimerDeleteValidCommand(wrongCapability, reason));
    assert(reason == "provider-selection-mismatch");

    auto wrongAuthority = valid;
    wrongAuthority.localProviderSelection.authorityDomain = "vdr.native";
    assert(!backendAgentNativeTimerDeleteValidCommand(wrongAuthority, reason));

    auto wrongProvider = valid;
    wrongProvider.localProviderSelection.providerId = "restfulapi:local";
    wrongProvider.localProviderSelection.providerKind = "restfulapi";
    assert(!backendAgentNativeTimerDeleteValidCommand(wrongProvider, reason));

    auto noFence = valid;
    noFence.controlPlaneClaimedAt = 0;
    assert(!backendAgentNativeTimerDeleteValidCommand(noFence, reason));

    auto accepted = evidence(
        BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified);
    assert(backendAgentNativeTimerDeleteEvidenceMatches(accepted, valid, reason));

    auto unknown = evidence(
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);
    assert(backendAgentNativeTimerDeleteEvidenceMatches(unknown, valid, reason));

    auto rejected = evidence(
        BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect);
    assert(backendAgentNativeTimerDeleteEvidenceMatches(rejected, valid, reason));

    auto rejectedWithDispatch = rejected;
    rejectedWithDispatch.dispatchStartedAt = 2070;
    assert(!backendAgentNativeTimerDeleteEvidenceMatches(
        rejectedWithDispatch, valid, reason));
    assert(reason == "rejected-outcome-has-dispatch");

    auto beforeStarting = accepted;
    beforeStarting.dispatchStartedAt = 2040;
    assert(!backendAgentNativeTimerDeleteEvidenceMatches(beforeStarting, valid, reason));
    assert(reason == "invalid-dispatch-boundary");

    auto startingBeforeClaim = accepted;
    startingBeforeClaim.localStartingPersistedAt = 1999;
    assert(!backendAgentNativeTimerDeleteEvidenceMatches(
        startingBeforeClaim, valid, reason));
    assert(reason == "invalid-evidence-timing");

    auto wrongOperation = accepted;
    wrongOperation.operationRevision = "3";
    assert(!backendAgentNativeTimerDeleteEvidenceMatches(wrongOperation, valid, reason));
    assert(reason == "evidence-identity-mismatch");

    auto wrongEpoch = accepted;
    wrongEpoch.providerInstanceEpoch = "plugin-epoch:10";
    assert(!backendAgentNativeTimerDeleteEvidenceMatches(wrongEpoch, valid, reason));

    std::cout << "Phase 64 native Timer delete Agent contract regression passed\n";
    return 0;
}

#include "BackendAgentCommandDelivery.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
BackendAgentCommandEnvelope command(const std::string& commandId="cmd-1")
{
    BackendAgentCommandEnvelope value;
    value.protocolVersion="vdr-suite-agent/1";
    value.requestId="req-1";
    value.correlationId="corr-1";
    value.operationId="op-1";
    value.jobId="job-1";
    value.attemptId="attempt-1";
    value.claimEpoch=4;
    value.commandId=commandId;
    value.backendId="default";
    value.agentId="agent-1";
    value.agentInstanceId="instance-1";
    value.backendGeneration=7;
    value.commandType="delivery-probe";
    value.payloadVersion=1;
    value.payload="non-mutating-fixture";
    value.requestFingerprint="fingerprint-1";
    value.assignedAt=1000;
    value.deadline=2000;
    return value;
}

BackendAgentCommandResult resultFor(const BackendAgentCommandEnvelope& value)
{
    BackendAgentCommandResult result;
    result.commandId=value.commandId;
    result.requestFingerprint=value.requestFingerprint;
    result.jobId=value.jobId;
    result.attemptId=value.attemptId;
    result.claimEpoch=value.claimEpoch;
    result.backendId=value.backendId;
    result.agentId=value.agentId;
    result.agentInstanceId=value.agentInstanceId;
    result.backendGeneration=value.backendGeneration;
    result.dispatchState=BackendAgentCommandDispatchState::EffectReported;
    result.verificationState="verified-non-mutating";
    result.resultCategory="succeeded";
    result.retryClassification="terminal";
    result.boundedDiagnostics="probe-only";
    result.completedAt=1100;
    return result;
}
}

int main()
{
    std::string reason;
    auto envelope=command();
    assert(backendAgentValidCommandEnvelope(envelope,reason));

    auto malformed=envelope;
    malformed.protocolVersion="vdr-suite-agent/2";
    assert(!backendAgentValidCommandEnvelope(malformed,reason));
    assert(reason=="unsupported_protocol");

    const auto directory=std::filesystem::temp_directory_path()/
        "vdr-suite-command-delivery-test";
    std::filesystem::remove_all(directory);
    BackendAgentLocalCommandStore store(directory.string());
    assert(store.ensurePrivateStateDirectory());

    auto accepted=store.accept(envelope,"agent-1","instance-1",7,1050);
    assert(accepted.accepted);
    assert(accepted.state.receiptPresent);
    assert(accepted.state.receipt.receiptCategory=="accepted");
    assert(accepted.state.dispatchState==
        BackendAgentCommandDispatchState::NotStarted);

    BackendAgentLocalCommandStore restarted(directory.string());
    const auto persisted=restarted.find(envelope.commandId);
    assert(persisted);
    assert(persisted->receiptPresent);
    assert(persisted->envelope.requestFingerprint=="fingerprint-1");

    const auto duplicate=restarted.accept(
        envelope,"agent-1","instance-1",7,1060);
    assert(duplicate.accepted);
    assert(duplicate.duplicate);

    auto conflict=envelope;
    conflict.requestFingerprint="fingerprint-conflict";
    const auto conflicting=restarted.accept(
        conflict,"agent-1","instance-1",7,1060);
    assert(!conflicting.accepted);
    assert(conflicting.conflict);
    assert(conflicting.reasonCode=="conflicting_duplicate");

    auto stale=envelope;
    stale.commandId="cmd-stale";
    stale.requestFingerprint="fingerprint-stale";
    const auto staleDecision=restarted.accept(
        stale,"agent-1","new-instance",8,1060);
    assert(!staleDecision.accepted);
    assert(staleDecision.stale);

    auto expired=envelope;
    expired.commandId="cmd-expired";
    expired.requestFingerprint="fingerprint-expired";
    const auto expiredDecision=restarted.accept(
        expired,"agent-1","instance-1",7,2001);
    assert(!expiredDecision.accepted);
    assert(expiredDecision.expired);

    assert(restarted.markStarting(envelope.commandId));
    assert(!restarted.markStarting(envelope.commandId));
    auto starting=restarted.find(envelope.commandId);
    assert(starting && starting->dispatchState==
        BackendAgentCommandDispatchState::Starting);

    assert(restarted.markAcceptedByExecutor(envelope.commandId));
    auto result=resultFor(envelope);
    assert(restarted.persistResult(result));
    assert(restarted.persistResult(result));

    auto conflictingResult=result;
    conflictingResult.boundedDiagnostics="different";
    assert(!restarted.persistResult(conflictingResult));

    const auto pending=restarted.pendingResults();
    assert(pending.size()==1);
    assert(pending.front().commandId==envelope.commandId);
    assert(restarted.acknowledgeResult(
        envelope.commandId,envelope.requestFingerprint));
    assert(restarted.pendingResults().empty());

    BackendAgentLocalCommandStore finalRestart(directory.string());
    const auto finalState=finalRestart.find(envelope.commandId);
    assert(finalState);
    assert(finalState->resultPresent);
    assert(finalState->resultAcknowledged);
    assert(finalState->dispatchState==
        BackendAgentCommandDispatchState::EffectReported);

    std::filesystem::remove_all(directory);
    std::cout<<"Backend Agent command delivery core tests passed\n";
    return 0;
}

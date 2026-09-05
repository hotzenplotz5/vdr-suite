#include "BackendAgentRecordingMarksModifyCommandHandler.h"
#include "BackendAgentRecordingMarksModifyPayload.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <string>
#include <unistd.h>

using namespace vdrsuite::agent;

namespace
{

std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

BackendAgentCommandAssignment assignment(std::int64_t now, const std::string& suffix)
{
    BackendAgentRecordingMarksModifyPayload payload;
    payload.kind = BackendAgentRecordingMarksModifyKind::replace;
    payload.operationRevision = "4";
    payload.recordingKey = "0123456789abcdef0123456789abcdef";
    payload.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    payload.replacementFrames = {10, 20, 30};
    payload.backendId = "default";
    payload.backendGeneration = 7;
    payload.controlPlaneClaimedAt = now - 20;
    payload.localProviderSelection.backendId = "default";
    payload.localProviderSelection.authorityDomain =
        kBackendAgentRecordingMarksModifyAuthorityDomain;
    payload.localProviderSelection.providerId =
        kBackendAgentRecordingMarksModifyProviderId;
    payload.localProviderSelection.providerKind =
        kBackendAgentRecordingMarksModifyProviderKind;
    payload.localProviderSelection.ownershipGeneration = 1;
    payload.localProviderSelection.providerInstanceEpoch = "pie_marks_handler";
    payload.localProviderSelection.providerGeneration = 2;
    payload.localProviderSelection.capabilityRevision = 3;
    payload.localProviderSelection.requiredCapability =
        kBackendAgentRecordingMarksModifyCapability;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_marks_" + suffix;
    value.correlationId = "corr_marks_" + suffix;
    value.operationId = "op_marks_" + suffix;
    value.jobId = "job_marks_" + suffix;
    value.attemptId = "attempt_marks_" + suffix;
    value.claimEpoch = 1;
    value.commandId = "cmd_marks_" + suffix;
    value.backendId = "default";
    value.agentId = "agent_marks";
    value.agentInstanceId = "instance_marks";
    value.backendGeneration = 7;
    value.commandType = kBackendAgentRecordingMarksModifyCommandType;
    value.payloadVersion = kBackendAgentRecordingMarksModifyPayloadVersion;
    value.payload = backendAgentRecordingMarksModifyPayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = now - 10;
    value.deadline = now + 300;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

commandstate::LocalState localState(
    std::int64_t now,
    const std::string& suffix)
{
    commandstate::LocalState state;
    state.assignment = assignment(now, suffix);
    auto& receipt = state.receipt;
    receipt.commandId = state.assignment.commandId;
    receipt.requestFingerprint = state.assignment.requestFingerprint;
    receipt.jobId = state.assignment.jobId;
    receipt.attemptId = state.assignment.attemptId;
    receipt.claimEpoch = state.assignment.claimEpoch;
    receipt.backendId = state.assignment.backendId;
    receipt.agentId = state.assignment.agentId;
    receipt.agentInstanceId = state.assignment.agentInstanceId;
    receipt.backendGeneration = state.assignment.backendGeneration;
    receipt.receiptCategory = "accepted";
    receipt.receivedAt = now;
    receipt.reasonCode = "durably_recorded";
    assert(backendAgentCommandValidReceipt(receipt));
    return state;
}

BackendAgentRecordingMarksModifyCommandContext context()
{
    return {"default", "agent_marks", "instance_marks", 7};
}

class MockTransport final : public IBackendAgentRecordingMarksModifyTransport
{
public:
    int discoverCalls = 0;
    int modifyCalls = 0;

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) override
    {
        ++discoverCalls;
        facts.providerId = kBackendAgentRecordingMarksModifyProviderId;
        facts.providerKind = kBackendAgentRecordingMarksModifyProviderKind;
        facts.providerInstanceEpoch = "pie_marks_handler";
        facts.providerGeneration = 2;
        facts.capabilityRevision = 3;
        facts.available = true;
        facts.capabilities = {kBackendAgentRecordingMarksModifyCapability};
        reasonCode.clear();
        return true;
    }

    BackendAgentRecordingMarksModifyTransportReply modifyMarks(
        const BackendAgentRecordingMarksModifyTransportRequest&) override
    {
        ++modifyCalls;
        BackendAgentRecordingMarksModifyTransportReply reply;
        reply.disposition =
            BackendAgentRecordingMarksModifyTransportDisposition::acceptedUnverified;
        reply.evidenceReference = "nmarks:handler:accepted";
        return reply;
    }
};

std::string statePath(const std::string& suffix)
{
    return "/tmp/vdr-suite-marks-handler-" +
        std::to_string(static_cast<long long>(getpid())) + "-" + suffix;
}

}

int main()
{
    const std::int64_t now = nowSeconds();
    std::string reasonCode;

    const std::string freshPath = statePath("fresh");
    std::remove(freshPath.c_str());
    std::remove((freshPath + ".tmp").c_str());
    auto fresh = localState(now, "fresh");
    assert(backendAgentRecordingMarksModifyCommandPrepareFreshStarting(
        freshPath, fresh, now, reasonCode));
    assert(fresh.dispatchState == "starting");
    assert(fresh.stateExtensionPresent);

    fresh.receiptAcknowledged = true;
    assert(commandstate::persist(freshPath, fresh, reasonCode));

    MockTransport transport;
    assert(backendAgentRecordingMarksModifyCommandExecuteFreshStartingAndPersistOutcome(
        freshPath, context(), &transport, fresh, reasonCode));
    assert(transport.discoverCalls == 1);
    assert(transport.modifyCalls == 1);
    assert(fresh.resultPresent);
    assert(fresh.result.dispatchState == "accepted_by_executor");
    assert(fresh.result.verificationState == "outcome_unknown");
    assert(fresh.result.resultCategory == "outcome_unknown");
    assert(fresh.result.errorCategory == "none");
    assert(fresh.result.retryClassification == "reconcile_only");

    commandstate::LocalState loadedFresh;
    assert(commandstate::load(freshPath, loadedFresh, reasonCode));
    assert(backendAgentRecordingMarksModifyCommandReconcileExisting(
        freshPath, context(), loadedFresh, reasonCode));
    assert(transport.modifyCalls == 1);
    assert(loadedFresh.resultPresent);
    assert(loadedFresh.result.retryClassification == "reconcile_only");

    const std::string recoveryPath = statePath("recovery");
    std::remove(recoveryPath.c_str());
    std::remove((recoveryPath + ".tmp").c_str());
    auto recovery = localState(now, "recovery");
    assert(backendAgentRecordingMarksModifyCommandPrepareFreshStarting(
        recoveryPath, recovery, now, reasonCode));

    commandstate::LocalState restarted;
    assert(commandstate::load(recoveryPath, restarted, reasonCode));
    assert(restarted.dispatchState == "starting");
    assert(!restarted.resultPresent);

    assert(backendAgentRecordingMarksModifyCommandReconcileExisting(
        recoveryPath, context(), restarted, reasonCode));
    assert(restarted.resultPresent);
    assert(restarted.result.dispatchState == "starting");
    assert(restarted.result.verificationState == "outcome_unknown");
    assert(restarted.result.resultCategory == "outcome_unknown");
    assert(restarted.result.errorCategory == "executor_unknown");
    assert(restarted.result.retryClassification == "reconcile_only");

    commandstate::LocalState restartedAgain;
    assert(commandstate::load(recoveryPath, restartedAgain, reasonCode));
    assert(backendAgentRecordingMarksModifyCommandReconcileExisting(
        recoveryPath, context(), restartedAgain, reasonCode));
    assert(restartedAgain.resultPresent);
    assert(restartedAgain.result.retryClassification == "reconcile_only");

    std::remove(freshPath.c_str());
    std::remove((freshPath + ".tmp").c_str());
    std::remove(recoveryPath.c_str());
    std::remove((recoveryPath + ".tmp").c_str());
    return 0;
}

#include "BackendAgentRecordingMarksModifyExecutor.h"
#include "BackendAgentRecordingMarksModifyPayload.h"

#include <cassert>
#include <stdexcept>

using namespace vdrsuite::agent;

namespace
{

BackendAgentRecordingMarksModifyPayload payload()
{
    BackendAgentRecordingMarksModifyPayload value;
    value.kind = BackendAgentRecordingMarksModifyKind::move;
    value.operationRevision = "5";
    value.recordingKey = "0123456789abcdef0123456789abcdef";
    value.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    value.sourceFrame = 100;
    value.targetFrame = 120;
    value.backendId = "default";
    value.backendGeneration = 7;
    value.controlPlaneClaimedAt = 100;
    auto& selection = value.localProviderSelection;
    selection.backendId = value.backendId;
    selection.authorityDomain = kBackendAgentRecordingMarksModifyAuthorityDomain;
    selection.providerId = kBackendAgentRecordingMarksModifyProviderId;
    selection.providerKind = kBackendAgentRecordingMarksModifyProviderKind;
    selection.ownershipGeneration = 1;
    selection.providerInstanceEpoch = "pie_marks_1";
    selection.providerGeneration = 2;
    selection.capabilityRevision = 3;
    selection.requiredCapability = kBackendAgentRecordingMarksModifyCapability;
    return value;
}

BackendAgentCommandAssignment assignment()
{
    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_marks_1";
    value.correlationId = "req_marks_1";
    value.operationId = "op_marks_1";
    value.commandType = kBackendAgentRecordingMarksModifyCommandType;
    value.payloadVersion = kBackendAgentRecordingMarksModifyPayloadVersion;
    value.payload = backendAgentRecordingMarksModifyPayload(payload());
    value.verificationPolicy = "readback_required";
    value.jobId = "job_marks_1";
    value.attemptId = "attempt_marks_1";
    value.claimEpoch = 1;
    value.commandId = "cmd_marks_1";
    value.backendId = "default";
    value.agentId = "agent_marks_1";
    value.agentInstanceId = "instance_marks_1";
    value.backendGeneration = 7;
    value.assignedAt = 100;
    value.deadline = 1000;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    return value;
}

BackendAgentRecordingMarksModifyLocalState starting(
    const BackendAgentCommandAssignment& value)
{
    BackendAgentRecordingMarksModifyLocalState state;
    std::string reasonCode;
    assert(backendAgentRecordingMarksModifyPrepareLocalStarting(
        value, 101, state, reasonCode));
    return state;
}

struct Transport final : IBackendAgentRecordingMarksModifyTransport
{
    int discoverCalls = 0;
    int modifyCalls = 0;
    bool staleProvider = false;
    bool throwOnModify = false;
    BackendAgentRecordingMarksModifyTransportDisposition disposition =
        BackendAgentRecordingMarksModifyTransportDisposition::acceptedUnverified;

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string&) override
    {
        ++discoverCalls;
        facts.providerId = kBackendAgentRecordingMarksModifyProviderId;
        facts.providerKind = kBackendAgentRecordingMarksModifyProviderKind;
        facts.providerInstanceEpoch = staleProvider ? "pie_marks_2" : "pie_marks_1";
        facts.providerGeneration = staleProvider ? 3 : 2;
        facts.capabilityRevision = staleProvider ? 4 : 3;
        facts.available = true;
        facts.capabilities = {kBackendAgentRecordingMarksModifyCapability};
        return true;
    }

    BackendAgentRecordingMarksModifyTransportReply modifyMarks(
        const BackendAgentRecordingMarksModifyTransportRequest&) override
    {
        ++modifyCalls;
        if (throwOnModify) throw std::runtime_error("transport failure");
        return {disposition, "nmarks:test:accepted"};
    }
};

}

int main()
{
    const auto commandAssignment = assignment();
    std::string reasonCode;

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        BackendAgentRecordingMarksModifyEvidence evidence;
        BackendAgentRecordingMarksModifyExecutorContext context{
            "default", "agent_marks_1", "instance_marks_1", 7, 102};
        assert(backendAgentRecordingMarksModifyExecuteFreshStartingOnce(
            commandAssignment,
            localState,
            context,
            transport,
            evidence,
            reasonCode));
        assert(transport.discoverCalls == 1);
        assert(transport.modifyCalls == 1);
        assert(evidence.outcome ==
            BackendAgentRecordingMarksModifyOutcomeCategory::acceptedUnverified);
        assert(evidence.dispatchStartedAt == 102);
        assert(backendAgentRecordingMarksModifyCompleteLocalState(
            localState, evidence, reasonCode));
    }

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        BackendAgentRecordingMarksModifyEvidence evidence;
        BackendAgentRecordingMarksModifyExecutorContext context{
            "default", "agent_marks_1", "replacement_instance", 7, 102};
        assert(backendAgentRecordingMarksModifyExecuteFreshStartingOnce(
            commandAssignment,
            localState,
            context,
            transport,
            evidence,
            reasonCode));
        assert(transport.discoverCalls == 0);
        assert(transport.modifyCalls == 0);
        assert(evidence.outcome ==
            BackendAgentRecordingMarksModifyOutcomeCategory::rejectedWithoutEffect);
    }

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        transport.staleProvider = true;
        BackendAgentRecordingMarksModifyEvidence evidence;
        BackendAgentRecordingMarksModifyExecutorContext context{
            "default", "agent_marks_1", "instance_marks_1", 7, 102};
        assert(backendAgentRecordingMarksModifyExecuteFreshStartingOnce(
            commandAssignment,
            localState,
            context,
            transport,
            evidence,
            reasonCode));
        assert(transport.discoverCalls == 1);
        assert(transport.modifyCalls == 0);
        assert(evidence.outcome ==
            BackendAgentRecordingMarksModifyOutcomeCategory::rejectedWithoutEffect);
    }

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        transport.throwOnModify = true;
        BackendAgentRecordingMarksModifyEvidence evidence;
        BackendAgentRecordingMarksModifyExecutorContext context{
            "default", "agent_marks_1", "instance_marks_1", 7, 102};
        assert(backendAgentRecordingMarksModifyExecuteFreshStartingOnce(
            commandAssignment,
            localState,
            context,
            transport,
            evidence,
            reasonCode));
        assert(transport.modifyCalls == 1);
        assert(evidence.outcome ==
            BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown);
    }

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        BackendAgentRecordingMarksModifyEvidence evidence;
        BackendAgentRecordingMarksModifyExecutorContext context{
            "default", "agent_marks_1", "instance_marks_1", 7, 1000};
        assert(backendAgentRecordingMarksModifyExecuteFreshStartingOnce(
            commandAssignment,
            localState,
            context,
            transport,
            evidence,
            reasonCode));
        assert(transport.modifyCalls == 0);
        assert(evidence.outcome ==
            BackendAgentRecordingMarksModifyOutcomeCategory::rejectedWithoutEffect);
    }

    return 0;
}

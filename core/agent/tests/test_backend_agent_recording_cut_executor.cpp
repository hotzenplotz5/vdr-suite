#include "BackendAgentRecordingCutExecutor.h"
#include "BackendAgentRecordingCutPayload.h"

#include <cassert>
#include <stdexcept>

using namespace vdrsuite::agent;

namespace
{
BackendAgentRecordingCutPayload payload()
{
    BackendAgentRecordingCutPayload value;
    value.operationRevision = "5";
    value.recordingKey = "0123456789abcdef0123456789abcdef";
    value.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.controlPlaneClaimedAt = 100;
    auto& selection = value.localProviderSelection;
    selection.backendId = value.backendId;
    selection.authorityDomain = kBackendAgentRecordingCutAuthorityDomain;
    selection.providerId = kBackendAgentRecordingCutProviderId;
    selection.providerKind = kBackendAgentRecordingCutProviderKind;
    selection.ownershipGeneration = 1;
    selection.providerInstanceEpoch = "pie_cut_1";
    selection.providerGeneration = 1;
    selection.capabilityRevision = 1;
    selection.requiredCapability = kBackendAgentRecordingCutCapability;
    return value;
}

BackendAgentCommandAssignment assignment()
{
    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_cut_1";
    value.correlationId = "req_cut_1";
    value.operationId = "op_cut_1";
    value.commandType = kBackendAgentRecordingCutCommandType;
    value.payloadVersion = kBackendAgentRecordingCutPayloadVersion;
    value.payload = backendAgentRecordingCutPayload(payload());
    value.verificationPolicy = "readback_required";
    value.jobId = "job_cut_1";
    value.attemptId = "attempt_cut_1";
    value.claimEpoch = 1;
    value.commandId = "cmd_cut_1";
    value.backendId = "default";
    value.agentId = "agent_cut_1";
    value.agentInstanceId = "instance_cut_1";
    value.backendGeneration = 7;
    value.assignedAt = 100;
    value.deadline = 1000;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    return value;
}

BackendAgentRecordingCutLocalState starting(
    const BackendAgentCommandAssignment& value)
{
    BackendAgentRecordingCutLocalState state;
    std::string reason;
    assert(backendAgentRecordingCutPrepareLocalStarting(value, 101, state, reason));
    return state;
}

struct Transport final : IBackendAgentRecordingCutTransport
{
    int discoverCalls = 0;
    int startCalls = 0;
    bool staleProvider = false;
    bool throwOnStart = false;
    BackendAgentRecordingCutTransportDisposition disposition =
        BackendAgentRecordingCutTransportDisposition::acceptedUnverified;

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string&) override
    {
        ++discoverCalls;
        facts.providerId = kBackendAgentRecordingCutProviderId;
        facts.providerKind = kBackendAgentRecordingCutProviderKind;
        facts.providerInstanceEpoch = staleProvider ? "pie_cut_2" : "pie_cut_1";
        facts.providerGeneration = staleProvider ? 2 : 1;
        facts.capabilityRevision = staleProvider ? 2 : 1;
        facts.available = true;
        facts.capabilities = {kBackendAgentRecordingCutCapability};
        return true;
    }

    BackendAgentRecordingCutTransportReply startCut(
        const BackendAgentRecordingCutTransportRequest&) override
    {
        ++startCalls;
        if (throwOnStart) throw std::runtime_error("transport failure");
        return {disposition, "ncut:test:accepted"};
    }
};
}

int main()
{
    const auto commandAssignment = assignment();
    std::string reason;

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        BackendAgentRecordingCutEvidence evidence;
        BackendAgentRecordingCutExecutorContext context{
            "default", "agent_cut_1", "instance_cut_1", 7, 102};
        assert(backendAgentRecordingCutExecuteFreshStartingOnce(
            commandAssignment, localState, context, transport, evidence, reason));
        assert(transport.discoverCalls == 1);
        assert(transport.startCalls == 1);
        assert(evidence.outcome ==
            BackendAgentRecordingCutOutcomeCategory::acceptedUnverified);
        assert(evidence.dispatchStartedAt == 102);
        assert(backendAgentRecordingCutCompleteLocalState(
            localState, evidence, reason));
    }

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        BackendAgentRecordingCutEvidence evidence;
        BackendAgentRecordingCutExecutorContext context{
            "default", "agent_cut_1", "replacement_instance", 7, 102};
        assert(backendAgentRecordingCutExecuteFreshStartingOnce(
            commandAssignment, localState, context, transport, evidence, reason));
        assert(transport.discoverCalls == 0);
        assert(transport.startCalls == 0);
        assert(evidence.outcome ==
            BackendAgentRecordingCutOutcomeCategory::rejectedWithoutEffect);
    }

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        transport.staleProvider = true;
        BackendAgentRecordingCutEvidence evidence;
        BackendAgentRecordingCutExecutorContext context{
            "default", "agent_cut_1", "instance_cut_1", 7, 102};
        assert(backendAgentRecordingCutExecuteFreshStartingOnce(
            commandAssignment, localState, context, transport, evidence, reason));
        assert(transport.discoverCalls == 1);
        assert(transport.startCalls == 0);
        assert(evidence.outcome ==
            BackendAgentRecordingCutOutcomeCategory::rejectedWithoutEffect);
    }

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        transport.throwOnStart = true;
        BackendAgentRecordingCutEvidence evidence;
        BackendAgentRecordingCutExecutorContext context{
            "default", "agent_cut_1", "instance_cut_1", 7, 102};
        assert(backendAgentRecordingCutExecuteFreshStartingOnce(
            commandAssignment, localState, context, transport, evidence, reason));
        assert(transport.startCalls == 1);
        assert(evidence.outcome ==
            BackendAgentRecordingCutOutcomeCategory::outcomeUnknown);
    }

    {
        auto localState = starting(commandAssignment);
        Transport transport;
        BackendAgentRecordingCutEvidence evidence;
        BackendAgentRecordingCutExecutorContext context{
            "default", "agent_cut_1", "instance_cut_1", 7, 1000};
        assert(backendAgentRecordingCutExecuteFreshStartingOnce(
            commandAssignment, localState, context, transport, evidence, reason));
        assert(transport.startCalls == 0);
        assert(evidence.outcome ==
            BackendAgentRecordingCutOutcomeCategory::rejectedWithoutEffect);
    }

    return 0;
}

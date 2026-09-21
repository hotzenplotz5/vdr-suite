#include "BackendAgentRecordingCutPayload.h"

#include <cassert>
#include <string>

using namespace vdrsuite::agent;

namespace
{
BackendAgentRecordingCutPayload payload()
{
    BackendAgentRecordingCutPayload value;
    value.operationRevision = "cutrev_1";
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
    selection.ownershipGeneration = 2;
    selection.providerInstanceEpoch = "pie_cut_1";
    selection.providerGeneration = 1;
    selection.capabilityRevision = 1;
    selection.requiredCapability = kBackendAgentRecordingCutCapability;
    return value;
}

BackendAgentRecordingCutCommand command(const BackendAgentRecordingCutPayload& p)
{
    BackendAgentRecordingCutCommand value;
    value.commandId = "cmd_cut_1";
    value.requestFingerprint = "fp1_aaaaaaaaaaaaaaaa";
    value.operationId = "op_cut_1";
    value.operationRevision = p.operationRevision;
    value.recordingKey = p.recordingKey;
    value.expectedMarksRevision = p.expectedMarksRevision;
    value.jobId = "job_cut_1";
    value.attemptId = "attempt_cut_1";
    value.claimEpoch = 1;
    value.backendId = p.backendId;
    value.agentId = "agent_cut_1";
    value.agentInstanceId = "instance_cut_1";
    value.backendGeneration = p.backendGeneration;
    value.controlPlaneClaimedAt = p.controlPlaneClaimedAt;
    value.localProviderSelection = p.localProviderSelection;
    return value;
}
}

int main()
{
    const auto original = payload();
    std::string reason;
    assert(backendAgentRecordingCutValidPayload(original, reason));
    const std::string encoded = backendAgentRecordingCutPayload(original);
    assert(!encoded.empty());

    BackendAgentRecordingCutPayload parsed;
    assert(backendAgentRecordingCutParsePayload(encoded, parsed, reason));
    assert(backendAgentRecordingCutPayload(parsed) == encoded);
    assert(parsed.recordingKey == original.recordingKey);
    assert(parsed.expectedMarksRevision == original.expectedMarksRevision);
    assert(backendAgentLocalProviderSameFence(
        parsed.localProviderSelection, original.localProviderSelection));
    assert(backendAgentRecordingCutValidCommand(command(original), reason));

    auto badRevision = original;
    badRevision.expectedMarksRevision = "stale";
    assert(!backendAgentRecordingCutValidPayload(badRevision, reason));

    auto wrongAuthority = original;
    wrongAuthority.localProviderSelection.authorityDomain = "vdr.recording.marks";
    assert(!backendAgentRecordingCutValidPayload(wrongAuthority, reason));

    assert(!backendAgentRecordingCutParsePayload(
        encoded + "garbage", parsed, reason));
    return 0;
}

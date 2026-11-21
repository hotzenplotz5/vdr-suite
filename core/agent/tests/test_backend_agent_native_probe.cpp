#include "BackendAgentNativeProbe.h"
#include <cassert>
#include <string>
using namespace vdrsuite::agent;
int main()
{
    SuiteBridgeNativeProbeCapability capability;
    std::string reason;
    assert(backendAgentNativeProbeParseCapability(
        "{\"nativeOperation\":\"vdr.native.probe\",\"nativeOperationSchema\":1,\"sideEffectClass\":\"none\",\"mutations\":\"disabled\",\"localProviderKind\":\"suitebridge\",\"pluginInstanceEpoch\":\"pie_1\"}",
        capability, reason));
    assert(reason == "native_capability_compatible");
    std::string nonce;
    assert(backendAgentNativeProbeParsePayload("{\"probeSchema\":1,\"probeNonce\":\"pbn_1\"}", nonce));
    assert(nonce == "pbn_1");

    const auto facts=backendAgentNativeProbeProviderFacts(capability);
    assert(backendAgentLocalProviderValidFacts(facts));
    assert(facts.providerId=="suitebridge:local");
    assert(facts.providerKind=="suitebridge");
    assert(facts.providerInstanceEpoch=="pie_1");
    assert(facts.providerGeneration==1);
    assert(facts.capabilityRevision==1);
    assert(facts.capabilities==std::vector<std::string>{"vdr.native.probe"});

    BackendAgentLocalProviderSelection selection;
    selection.backendId="default";
    selection.authorityDomain="vdr.native";
    selection.providerId="suitebridge:local";
    selection.providerKind="suitebridge";
    selection.ownershipGeneration=4;
    selection.providerInstanceEpoch="pie_1";
    selection.providerGeneration=1;
    selection.capabilityRevision=1;
    selection.requiredCapability="vdr.native.probe";
    const std::string selectedPayload=
        backendAgentNativeProbeSelectedPayload("pbn_2",selection);
    assert(!selectedPayload.empty());
    BackendAgentNativeProbePayload parsedSelected;
    assert(backendAgentNativeProbeParseSelectedPayload(
        selectedPayload,parsedSelected,reason));
    assert(reason=="native_probe_selected_payload_parsed");
    assert(parsedSelected.probeNonce=="pbn_2");
    assert(backendAgentLocalProviderSameFence(
        selection,parsedSelected.localProviderSelection));
    assert(backendAgentNativeProbeSelectionMatchesCapability(
        parsedSelected.localProviderSelection,"default",capability,reason));
    assert(reason=="local_provider_fence_current");
    auto staleSelection=selection;
    staleSelection.providerInstanceEpoch="pie_2";
    assert(!backendAgentNativeProbeSelectionMatchesCapability(
        staleSelection,"default",capability,reason));
    assert(reason=="local_provider_instance_epoch_changed");

    SuiteBridgeNativeProbeEvidence execution;
    const std::string payload =
        "{\"commandId\":\"cmd_1\",\"requestFingerprint\":\"fp_1\",\"nativeOperation\":\"vdr.native.probe\",\"nativeOperationSchema\":1,\"pluginInstanceEpoch\":\"pie_1\",\"nativeExecutionSequence\":3,\"receiptCategory\":\"accepted\",\"acceptedAt\":10,\"sideEffectClass\":\"none\",\"resultCategory\":\"succeeded\",\"vdrActive\":true,\"mutationsState\":\"disabled\",\"sideEffectObserved\":false,\"boundedDiagnostics\":\"native probe completed\",\"completedAt\":11}";
    assert(backendAgentNativeProbeParseEvidence(payload, false, execution, reason));
    SuiteBridgeNativeProbeRequest request;
    request.commandId="cmd_1";request.requestFingerprint="fp_1";
    request.pluginInstanceEpoch="pie_1";
    assert(backendAgentNativeProbeEvidenceMatches(execution, request, false));

    SuiteBridgeNativeProbeEvidence readback;
    const std::string readbackPayload = payload.substr(0, payload.size()-1) +
        ",\"readbackCategory\":\"verified\",\"duplicateDisposition\":\"exact_replay\"}";
    assert(backendAgentNativeProbeParseEvidence(readbackPayload, true, readback, reason));
    assert(readback.nativeExecutionSequence == execution.nativeExecutionSequence);
    assert(backendAgentNativeProbeEvidenceMatches(readback, request, true));

    SuiteBridgeNativeProbeEvidence bad;
    std::string badPayload = readbackPayload;
    const auto pos = badPayload.find("\"vdrActive\":true");
    badPayload.replace(pos, std::string("\"vdrActive\":true").size(), "\"vdrActive\":false");
    assert(!backendAgentNativeProbeParseEvidence(badPayload, true, bad, reason));
    assert(reason == "native_evidence_invariant_failed");
    return 0;
}

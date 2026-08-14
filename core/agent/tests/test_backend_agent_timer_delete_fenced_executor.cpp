#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerDeleteExecutor.h"
#include "BackendAgentNativeTimerDeletePayload.h"

#include <cassert>
#include <stdexcept>
#include <string>

using namespace vdrsuite::agent;

namespace
{

BackendAgentCommandAssignment assignment()
{
    BackendAgentLocalProviderSelection selection;
    selection.backendId = "default";
    selection.authorityDomain = kBackendAgentNativeTimerDeleteAuthorityDomain;
    selection.providerId = kBackendAgentNativeTimerDeleteProviderId;
    selection.providerKind = kBackendAgentNativeTimerDeleteProviderKind;
    selection.ownershipGeneration = 41;
    selection.providerInstanceEpoch = "suitebridge-epoch-executor";
    selection.providerGeneration = 13;
    selection.capabilityRevision = 6;
    selection.requiredCapability = kBackendAgentNativeTimerDeleteCapability;

    BackendAgentNativeTimerDeletePayload payload;
    payload.operationRevision = "operation-revision-executor";
    payload.nativeTimerBindingId = "binding-executor";
    payload.expectedBindingRevision = "binding-revision-executor";
    payload.expectedNativeTimerFingerprint = "sha256:native-timer-observed-executor";
    payload.timerAssignmentId = "timer-assignment-executor";
    payload.backendNativeTimerId = "native-timer-executor";
    payload.controlPlaneClaimedAt = 90;
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-executor";
    value.correlationId = "correlation-executor";
    value.operationId = "operation-executor";
    value.jobId = "job-executor";
    value.attemptId = "attempt-executor";
    value.claimEpoch = 5;
    value.commandId = "command-executor";
    value.backendId = "default";
    value.agentId = "agent-executor";
    value.agentInstanceId = "instance-executor";
    value.backendGeneration = 61;
    value.commandType = kBackendAgentNativeTimerDeleteCommandType;
    value.payloadVersion = kBackendAgentNativeTimerDeletePayloadVersion;
    value.payload = backendAgentNativeTimerDeletePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 100;
    value.deadline = 1000;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentNativeTimerDeleteLocalState starting(
    const BackendAgentCommandAssignment& value)
{
    BackendAgentNativeTimerDeleteLocalState state;
    std::string reason;
    assert(backendAgentNativeTimerDeletePrepareLocalStarting(
        value, 200, state, reason));
    assert(state.phase == BackendAgentNativeTimerDeleteLocalPhase::starting);
    return state;
}

BackendAgentLocalProviderFacts factsFor(
    const BackendAgentNativeTimerDeleteLocalState& state)
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = state.command.localProviderSelection.providerId;
    facts.providerKind = state.command.localProviderSelection.providerKind;
    facts.providerInstanceEpoch =
        state.command.localProviderSelection.providerInstanceEpoch;
    facts.providerGeneration =
        state.command.localProviderSelection.providerGeneration;
    facts.capabilityRevision =
        state.command.localProviderSelection.capabilityRevision;
    facts.available = true;
    facts.capabilities = {
        state.command.localProviderSelection.requiredCapability};
    assert(backendAgentLocalProviderValidFacts(facts));
    return facts;
}

BackendAgentNativeTimerDeleteExecutorContext contextFor(
    const BackendAgentCommandAssignment& value)
{
    return {
        value.backendId,
        value.agentId,
        value.agentInstanceId,
        value.backendGeneration,
        220};
}

class Transport final : public IBackendAgentNativeTimerDeleteTransport
{
public:
    BackendAgentLocalProviderFacts facts;
    bool discoverySucceeds = true;
    bool throwDiscovery = false;
    bool throwDelete = false;
    int discoveryCalls = 0;
    int deleteCalls = 0;
    BackendAgentNativeTimerDeleteTransportReply reply;

    bool discoverProvider(
        BackendAgentLocalProviderFacts& output,
        std::string& reason) override
    {
        ++discoveryCalls;
        if (throwDiscovery) throw std::runtime_error("discovery");
        if (!discoverySucceeds)
        {
            reason = "provider_unavailable";
            return false;
        }
        output = facts;
        reason = "provider_current";
        return true;
    }

    BackendAgentNativeTimerDeleteTransportReply deleteTimer(
        const BackendAgentNativeTimerDeleteTransportRequest& request) override
    {
        ++deleteCalls;
        assert(request.command.commandId == "command-executor");
        assert(request.command.expectedNativeTimerFingerprint ==
               "sha256:native-timer-observed-executor");
        assert(request.localStartingPersistedAt == 200);
        if (throwDelete) throw std::runtime_error("delete");
        return reply;
    }
};

void assertEvidence(
    const BackendAgentNativeTimerDeleteEvidence& evidence,
    const BackendAgentNativeTimerDeleteLocalState& state,
    BackendAgentNativeTimerDeleteOutcomeCategory outcome)
{
    std::string reason;
    assert(evidence.outcome == outcome);
    assert(backendAgentNativeTimerDeleteEvidenceMatches(
        evidence, state.command, reason));
}

}

int main()
{
    std::string reason;
    BackendAgentNativeTimerDeleteEvidence evidence;
    const auto commandAssignment = assignment();
    const auto localStarting = starting(commandAssignment);
    const auto context = contextFor(commandAssignment);

    // Exact persisted selection + current facts allows exactly one typed
    // dispatch. Accepted transport evidence is still unverified until later
    // authoritative absence readback.
    Transport accepted;
    accepted.facts = factsFor(localStarting);
    accepted.reply.disposition =
        BackendAgentNativeTimerDeleteTransportDisposition::acceptedUnverified;
    accepted.reply.evidenceReference = "suitebridge:accepted:1";
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, accepted, evidence, reason));
    assert(reason == "native_timer_delete_executor_accepted_unverified");
    assert(accepted.discoveryCalls == 1);
    assert(accepted.deleteCalls == 1);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified);
    assert(evidence.dispatchStartedAt == context.now);

    // Agent/backend-generation drift is a definitive pre-dispatch fence.
    Transport stale;
    stale.facts = factsFor(localStarting);
    auto staleContext = context;
    ++staleContext.backendGeneration;
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, staleContext, stale, evidence, reason));
    assert(reason == "native_timer_delete_executor_rejected_without_effect");
    assert(stale.discoveryCalls == 0);
    assert(stale.deleteCalls == 0);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect);
    assert(evidence.dispatchStartedAt == 0);

    // Deadline is checked again immediately before provider discovery.
    Transport expired;
    expired.facts = factsFor(localStarting);
    auto expiredContext = context;
    expiredContext.now = commandAssignment.deadline;
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, expiredContext, expired, evidence, reason));
    assert(expired.discoveryCalls == 0);
    assert(expired.deleteCalls == 0);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect);

    // Provider availability never creates authority. Every mutable provider
    // fence captured by the persisted selection must still match.
    Transport changedEpoch;
    changedEpoch.facts = factsFor(localStarting);
    changedEpoch.facts.providerInstanceEpoch = "suitebridge-epoch-changed";
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, changedEpoch, evidence, reason));
    assert(changedEpoch.discoveryCalls == 1);
    assert(changedEpoch.deleteCalls == 0);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect);

    Transport changedGeneration;
    changedGeneration.facts = factsFor(localStarting);
    ++changedGeneration.facts.providerGeneration;
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, changedGeneration, evidence, reason));
    assert(changedGeneration.deleteCalls == 0);

    Transport changedRevision;
    changedRevision.facts = factsFor(localStarting);
    ++changedRevision.facts.capabilityRevision;
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, changedRevision, evidence, reason));
    assert(changedRevision.deleteCalls == 0);

    Transport capabilityLost;
    capabilityLost.facts = factsFor(localStarting);
    capabilityLost.facts.capabilities = {"vdr.timer.read"};
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, capabilityLost, evidence, reason));
    assert(capabilityLost.deleteCalls == 0);

    Transport unavailable;
    unavailable.facts = factsFor(localStarting);
    unavailable.facts.available = false;
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, unavailable, evidence, reason));
    assert(unavailable.deleteCalls == 0);

    Transport discoveryFailure;
    discoveryFailure.facts = factsFor(localStarting);
    discoveryFailure.discoverySucceeds = false;
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, discoveryFailure, evidence, reason));
    assert(discoveryFailure.discoveryCalls == 1);
    assert(discoveryFailure.deleteCalls == 0);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect);

    // A transport may definitively reject without effect after receiving the
    // typed request; the executor still never retries it.
    Transport rejected;
    rejected.facts = factsFor(localStarting);
    rejected.reply.disposition =
        BackendAgentNativeTimerDeleteTransportDisposition::rejectedWithoutEffect;
    rejected.reply.evidenceReference = "suitebridge:rejected:no-effect";
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, rejected, evidence, reason));
    assert(rejected.deleteCalls == 1);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect);
    assert(evidence.dispatchStartedAt == 0);

    // Once dispatch has begun, exceptions, explicit ambiguity, or malformed
    // evidence are always outcome_unknown and reconciliation-only upstream.
    Transport exception;
    exception.facts = factsFor(localStarting);
    exception.throwDelete = true;
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, exception, evidence, reason));
    assert(exception.deleteCalls == 1);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);
    assert(evidence.dispatchStartedAt == context.now);

    Transport unknown;
    unknown.facts = factsFor(localStarting);
    unknown.reply.disposition =
        BackendAgentNativeTimerDeleteTransportDisposition::outcomeUnknown;
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context, unknown, evidence, reason));
    assert(unknown.deleteCalls == 1);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);

    Transport invalidAcceptedEvidence;
    invalidAcceptedEvidence.facts = factsFor(localStarting);
    invalidAcceptedEvidence.reply.disposition =
        BackendAgentNativeTimerDeleteTransportDisposition::acceptedUnverified;
    invalidAcceptedEvidence.reply.evidenceReference = "invalid\nreference";
    assert(backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, localStarting, context,
        invalidAcceptedEvidence, evidence, reason));
    assert(invalidAcceptedEvidence.deleteCalls == 1);
    assertEvidence(
        evidence, localStarting,
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);

    // Completed local state is historical evidence, never fresh execution
    // authority. The executor fails closed without provider discovery.
    auto completed = localStarting;
    BackendAgentNativeTimerDeleteEvidence completedEvidence;
    completedEvidence.commandId = completed.command.commandId;
    completedEvidence.requestFingerprint = completed.command.requestFingerprint;
    completedEvidence.operationId = completed.command.operationId;
    completedEvidence.operationRevision = completed.command.operationRevision;
    completedEvidence.jobId = completed.command.jobId;
    completedEvidence.attemptId = completed.command.attemptId;
    completedEvidence.claimEpoch = completed.command.claimEpoch;
    completedEvidence.backendId = completed.command.backendId;
    completedEvidence.agentId = completed.command.agentId;
    completedEvidence.agentInstanceId = completed.command.agentInstanceId;
    completedEvidence.backendGeneration = completed.command.backendGeneration;
    completedEvidence.providerInstanceEpoch =
        completed.command.localProviderSelection.providerInstanceEpoch;
    completedEvidence.localStartingPersistedAt = completed.localStartingPersistedAt;
    completedEvidence.outcome =
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown;
    completedEvidence.dispatchStartedAt = completed.localStartingPersistedAt;
    completedEvidence.completedAt = context.now;
    completedEvidence.evidenceReference = "prior:completed:evidence";
    assert(backendAgentNativeTimerDeleteCompleteLocalState(
        completed, completedEvidence, reason));

    Transport completedTransport;
    completedTransport.facts = factsFor(localStarting);
    assert(!backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
        commandAssignment, completed, context,
        completedTransport, evidence, reason));
    assert(reason == "native_timer_delete_executor_fresh_state_invalid");
    assert(completedTransport.discoveryCalls == 0);
    assert(completedTransport.deleteCalls == 0);

    return 0;
}

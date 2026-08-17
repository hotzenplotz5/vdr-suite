#include "BackendAgentCommand.h"
#include "BackendAgentCommandStateStore.h"
#include "BackendAgentNativeTimerCreateCommandHandler.h"
#include "BackendAgentNativeTimerCreateExecutor.h"
#include "BackendAgentNativeTimerCreatePayload.h"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <stdexcept>
#include <string>

using namespace vdrsuite::agent;

namespace
{

std::int64_t now()
{
    return static_cast<std::int64_t>(std::time(nullptr));
}

BackendAgentNativeTimerCreateSpecification specification()
{
    BackendAgentNativeTimerCreateSpecification value;
    value.channelId = "C-1-2-3";
    value.title = "Phase 64 CREATE executor";
    value.directory = "Tests";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "1930";
    value.endTime = "2030";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

BackendAgentCommandAssignment assignment(
    const std::string& suffix,
    std::int64_t current)
{
    BackendAgentLocalProviderSelection selection;
    selection.backendId = "default";
    selection.authorityDomain =
        kBackendAgentNativeTimerCreateAuthorityDomain;
    selection.providerId =
        kBackendAgentNativeTimerCreateProviderId;
    selection.providerKind =
        kBackendAgentNativeTimerCreateProviderKind;
    selection.ownershipGeneration = 41;
    selection.providerInstanceEpoch =
        "suitebridge-create-executor-" + suffix;
    selection.providerGeneration = 13;
    selection.capabilityRevision = 6;
    selection.requiredCapability =
        kBackendAgentNativeTimerCreateCapability;

    BackendAgentNativeTimerCreatePayload payload;
    payload.operationRevision = "2";
    payload.timerAssignmentId = "timer-assignment-" + suffix;
    payload.expectedAssignmentRevision = "4";
    payload.expectedIntentRevision = "9";
    payload.assignmentEpoch = 3;
    payload.nativeTimerBindingId = "binding-" + suffix;
    payload.controlPlaneClaimedAt = current - 20;
    payload.specification = specification();
    payload.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(
            payload.specification);
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-" + suffix;
    value.correlationId = "correlation-" + suffix;
    value.operationId = "operation-" + suffix;
    value.jobId = "job-" + suffix;
    value.attemptId = "attempt-" + suffix;
    value.claimEpoch = 5;
    value.commandId = "command-" + suffix;
    value.backendId = "default";
    value.agentId = "agent-" + suffix;
    value.agentInstanceId = "instance-" + suffix;
    value.backendGeneration = 61;
    value.commandType =
        kBackendAgentNativeTimerCreateCommandType;
    value.payloadVersion =
        kBackendAgentNativeTimerCreatePayloadVersion;
    value.payload =
        backendAgentNativeTimerCreatePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = current - 10;
    value.deadline = current + 600;
    value.requestFingerprint =
        backendAgentCommandFingerprint(value);

    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentNativeTimerCreateLocalState starting(
    const BackendAgentCommandAssignment& value,
    std::int64_t current)
{
    BackendAgentNativeTimerCreateCommand command;
    std::string reason;

    assert(backendAgentNativeTimerCreateCommandFromAssignment(
        value, command, reason));

    BackendAgentNativeTimerCreateLocalState state;

    assert(backendAgentNativeTimerCreatePrepareLocalStarting(
        command, current, state, reason));

    assert(
        state.phase ==
        BackendAgentNativeTimerCreateLocalPhase::starting);

    return state;
}

BackendAgentLocalProviderFacts factsFor(
    const BackendAgentNativeTimerCreateLocalState& state)
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId =
        state.command.localProviderSelection.providerId;
    facts.providerKind =
        state.command.localProviderSelection.providerKind;
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

BackendAgentNativeTimerCreateExecutorContext contextFor(
    const BackendAgentCommandAssignment& value,
    std::int64_t current)
{
    return {
        value.backendId,
        value.agentId,
        value.agentInstanceId,
        value.backendGeneration,
        current};
}

BackendAgentCommandReceipt receiptFor(
    const BackendAgentCommandAssignment& value,
    std::int64_t current)
{
    BackendAgentCommandReceipt receipt;
    receipt.protocolVersion = "vdr-suite-agent/1";
    receipt.commandId = value.commandId;
    receipt.requestFingerprint = value.requestFingerprint;
    receipt.jobId = value.jobId;
    receipt.attemptId = value.attemptId;
    receipt.claimEpoch = value.claimEpoch;
    receipt.backendId = value.backendId;
    receipt.agentId = value.agentId;
    receipt.agentInstanceId = value.agentInstanceId;
    receipt.backendGeneration = value.backendGeneration;
    receipt.receiptCategory = "accepted";
    receipt.receivedAt = current;
    receipt.reasonCode = "durably_recorded";

    assert(backendAgentCommandValidReceipt(receipt));
    return receipt;
}

class Transport final
    : public IBackendAgentNativeTimerCreateTransport
{
public:
    BackendAgentLocalProviderFacts facts;
    bool discoverySucceeds = true;
    bool throwDiscovery = false;
    bool throwCreate = false;
    int discoveryCalls = 0;
    int createCalls = 0;

    BackendAgentNativeTimerCreateTransportReply reply;

    bool discoverProvider(
        BackendAgentLocalProviderFacts& output,
        std::string& reason) override
    {
        ++discoveryCalls;

        if (throwDiscovery)
            throw std::runtime_error("discovery");

        if (!discoverySucceeds)
        {
            reason = "provider_unavailable";
            return false;
        }

        output = facts;
        reason = "provider_current";
        return true;
    }

    BackendAgentNativeTimerCreateTransportReply createTimer(
        const BackendAgentNativeTimerCreateTransportRequest& request)
        override
    {
        ++createCalls;

        assert(!request.command.commandId.empty());
        assert(
            request.command.expectedSpecificationFingerprint ==
            backendAgentNativeTimerCreateSpecificationFingerprint(
                request.command.specification));
        assert(request.localStartingPersistedAt > 0);

        if (throwCreate)
            throw std::runtime_error("create");

        return reply;
    }
};

void assertEvidence(
    const BackendAgentNativeTimerCreateEvidence& evidence,
    const BackendAgentNativeTimerCreateLocalState& state,
    BackendAgentNativeTimerCreateOutcomeCategory outcome)
{
    std::string reason;

    assert(evidence.outcome == outcome);
    assert(backendAgentNativeTimerCreateEvidenceMatches(
        evidence, state.command, reason));
}

std::string statePath(const std::string& suffix)
{
    return "/tmp/vdr-suite-phase64-create-" + suffix + ".state";
}

void cleanup(const std::string& path)
{
    std::remove(path.c_str());
    std::remove((path + ".tmp").c_str());
}

} // namespace

int main()
{
    const std::int64_t current = now();
    std::string reason;
    BackendAgentNativeTimerCreateEvidence evidence;

    const auto assigned =
        assignment("executor", current);
    const auto localStarting =
        starting(assigned, current);
    const auto context =
        contextFor(assigned, current + 1);

    // Exact persisted provider selection permits one typed dispatch.
    Transport accepted;
    accepted.facts = factsFor(localStarting);
    accepted.reply.disposition =
        BackendAgentNativeTimerCreateTransportDisposition::
            acceptedUnverified;
    accepted.reply.evidenceReference =
        "suitebridge:create:accepted:1";

    assert(backendAgentNativeTimerCreateExecuteFreshStartingOnce(
        assigned,
        localStarting,
        context,
        accepted,
        evidence,
        reason));

    assert(
        reason ==
        "native_timer_create_executor_accepted_unverified");
    assert(accepted.discoveryCalls == 1);
    assert(accepted.createCalls == 1);

    assertEvidence(
        evidence,
        localStarting,
        BackendAgentNativeTimerCreateOutcomeCategory::
            acceptedUnverified);

    assert(evidence.dispatchStartedAt == context.now);
    assert(
        evidence.nativeTimerBindingId ==
        localStarting.command.nativeTimerBindingId);

    // Agent/backend generation drift fences before discovery.
    Transport stale;
    stale.facts = factsFor(localStarting);

    auto staleContext = context;
    ++staleContext.backendGeneration;

    assert(backendAgentNativeTimerCreateExecuteFreshStartingOnce(
        assigned,
        localStarting,
        staleContext,
        stale,
        evidence,
        reason));

    assert(stale.discoveryCalls == 0);
    assert(stale.createCalls == 0);

    assertEvidence(
        evidence,
        localStarting,
        BackendAgentNativeTimerCreateOutcomeCategory::
            rejectedWithoutEffect);

    assert(evidence.dispatchStartedAt == 0);

    // Deadline is rechecked immediately before discovery.
    Transport expired;
    expired.facts = factsFor(localStarting);

    auto expiredContext = context;
    expiredContext.now = assigned.deadline;

    assert(backendAgentNativeTimerCreateExecuteFreshStartingOnce(
        assigned,
        localStarting,
        expiredContext,
        expired,
        evidence,
        reason));

    assert(expired.discoveryCalls == 0);
    assert(expired.createCalls == 0);

    // Provider epoch/generation/revision remains a hard fence.
    Transport changedProvider;
    changedProvider.facts = factsFor(localStarting);
    changedProvider.facts.providerInstanceEpoch =
        "suitebridge-create-changed";

    assert(backendAgentNativeTimerCreateExecuteFreshStartingOnce(
        assigned,
        localStarting,
        context,
        changedProvider,
        evidence,
        reason));

    assert(changedProvider.discoveryCalls == 1);
    assert(changedProvider.createCalls == 0);

    // Provider discovery failure is still pre-dispatch/no-effect.
    Transport discoveryFailure;
    discoveryFailure.facts = factsFor(localStarting);
    discoveryFailure.discoverySucceeds = false;

    assert(backendAgentNativeTimerCreateExecuteFreshStartingOnce(
        assigned,
        localStarting,
        context,
        discoveryFailure,
        evidence,
        reason));

    assert(discoveryFailure.discoveryCalls == 1);
    assert(discoveryFailure.createCalls == 0);

    assertEvidence(
        evidence,
        localStarting,
        BackendAgentNativeTimerCreateOutcomeCategory::
            rejectedWithoutEffect);

    // Once native dispatch starts, an exception can never authorize retry.
    Transport exception;
    exception.facts = factsFor(localStarting);
    exception.throwCreate = true;

    assert(backendAgentNativeTimerCreateExecuteFreshStartingOnce(
        assigned,
        localStarting,
        context,
        exception,
        evidence,
        reason));

    assert(exception.createCalls == 1);

    assertEvidence(
        evidence,
        localStarting,
        BackendAgentNativeTimerCreateOutcomeCategory::
            outcomeUnknown);

    assert(evidence.dispatchStartedAt == context.now);

    // Invalid accepted evidence is also outcome_unknown.
    Transport invalidAccepted;
    invalidAccepted.facts = factsFor(localStarting);
    invalidAccepted.reply.disposition =
        BackendAgentNativeTimerCreateTransportDisposition::
            acceptedUnverified;
    invalidAccepted.reply.evidenceReference =
        "invalid\nreference";

    assert(backendAgentNativeTimerCreateExecuteFreshStartingOnce(
        assigned,
        localStarting,
        context,
        invalidAccepted,
        evidence,
        reason));

    assert(invalidAccepted.createCalls == 1);

    assertEvidence(
        evidence,
        localStarting,
        BackendAgentNativeTimerCreateOutcomeCategory::
            outcomeUnknown);

    // Handler path: persist local starting, acknowledge receipt,
    // dispatch once, and durably persist typed evidence/result.
    {
        const std::string path =
            statePath("handler-accepted");
        cleanup(path);

        const auto handlerAssignment =
            assignment("handleraccepted", current);

        commandstate::LocalState state;
        state.assignment = handlerAssignment;
        state.receipt =
            receiptFor(handlerAssignment, current);

        assert(
            backendAgentNativeTimerCreateCommandPrepareFreshStarting(
                path,
                state,
                current,
                reason));

        assert(state.stateExtensionPresent);
        assert(state.dispatchState == "starting");

        state.receiptAcknowledged = true;
        assert(commandstate::persist(path, state, reason));

        BackendAgentNativeTimerCreateLocalState handlerStarting;
        assert(backendAgentNativeTimerCreateParseLocalState(
            state.stateExtension.payload,
            handlerStarting,
            reason));

        Transport handlerTransport;
        handlerTransport.facts =
            factsFor(handlerStarting);
        handlerTransport.reply.disposition =
            BackendAgentNativeTimerCreateTransportDisposition::
                acceptedUnverified;
        handlerTransport.reply.evidenceReference =
            "suitebridge:create:handler:accepted";

        BackendAgentNativeTimerCreateCommandContext handlerContext{
            handlerAssignment.backendId,
            handlerAssignment.agentId,
            handlerAssignment.agentInstanceId,
            handlerAssignment.backendGeneration};

        assert(
            backendAgentNativeTimerCreateCommandExecuteFreshStartingAndPersistOutcome(
                path,
                handlerContext,
                &handlerTransport,
                state,
                reason));

        assert(
            reason ==
            "native_create_executor_outcome_persisted");
        assert(handlerTransport.createCalls == 1);
        assert(state.resultPresent);
        assert(
            state.dispatchState ==
            "accepted_by_executor");
        assert(
            state.result.retryClassification ==
            "reconcile_only");

        commandstate::LocalState persisted;
        assert(commandstate::load(
            path, persisted, reason));
        assert(persisted.resultPresent);
        assert(persisted.stateExtensionPresent);

        BackendAgentNativeTimerCreateLocalState completed;
        assert(backendAgentNativeTimerCreateParseLocalState(
            persisted.stateExtension.payload,
            completed,
            reason));

        assert(
            completed.phase ==
            BackendAgentNativeTimerCreateLocalPhase::
                completed);
        assert(
            completed.evidence.outcome ==
            BackendAgentNativeTimerCreateOutcomeCategory::
                acceptedUnverified);

        cleanup(path);
    }

    // Crash boundary: once starting is durable, recovery completes it as
    // outcome_unknown/reconcile_only and must never call CREATE again.
    {
        const std::string path =
            statePath("handler-recovery");
        cleanup(path);

        const auto crashAssignment =
            assignment("handlerrecovery", current);

        commandstate::LocalState state;
        state.assignment = crashAssignment;
        state.receipt =
            receiptFor(crashAssignment, current);

        assert(
            backendAgentNativeTimerCreateCommandPrepareFreshStarting(
                path,
                state,
                current,
                reason));

        BackendAgentNativeTimerCreateCommandContext crashContext{
            crashAssignment.backendId,
            crashAssignment.agentId,
            crashAssignment.agentInstanceId,
            crashAssignment.backendGeneration};

        assert(
            backendAgentNativeTimerCreateCommandReconcileExisting(
                path,
                crashContext,
                state,
                reason));

        assert(state.resultPresent);
        assert(state.dispatchState == "starting");
        assert(
            state.result.retryClassification ==
            "reconcile_only");
        assert(
            state.result.resultCategory ==
            "outcome_unknown");

        BackendAgentNativeTimerCreateLocalState recovered;
        assert(backendAgentNativeTimerCreateParseLocalState(
            state.stateExtension.payload,
            recovered,
            reason));

        assert(
            recovered.phase ==
            BackendAgentNativeTimerCreateLocalPhase::
                completed);
        assert(
            recovered.evidence.outcome ==
            BackendAgentNativeTimerCreateOutcomeCategory::
                outcomeUnknown);

        Transport forbiddenReplay;
        forbiddenReplay.facts =
            factsFor(recovered);

        state.receiptAcknowledged = true;

        assert(
            !backendAgentNativeTimerCreateCommandExecuteFreshStartingAndPersistOutcome(
                path,
                crashContext,
                &forbiddenReplay,
                state,
                reason));

        assert(forbiddenReplay.discoveryCalls == 0);
        assert(forbiddenReplay.createCalls == 0);

        cleanup(path);
    }

    return 0;
}

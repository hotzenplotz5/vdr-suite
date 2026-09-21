#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingCutAssignment.h"
#include "Database.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* SourceRecordingKey =
    "0123456789abcdef0123456789abcdef";
constexpr const char* MarksRevision =
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
constexpr const char* EditedRecordingKey =
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
constexpr const char* WrongEditedRecordingKey =
    "cccccccccccccccccccccccccccccccc";

RequestSecurityContext systemContext()
{
    RequestSecurityContext value;
    value.requestId = "req_cut_reconciliation";
    value.correlationId = "corr_cut_reconciliation";
    value.authenticationState = AuthenticationState::Authenticated;
    value.actor = ActorIdentity{
        "system_cut_reconciliation", ActorType::System, "test", true};
    value.device = DeviceIdentity{"dev_cut_reconciliation", true};
    value.credential = CredentialIdentity{
        "cred_cut_reconciliation", true, false, false};
    value.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return value;
}

vdrsuite::agent::BackendAgentLocalProviderFacts providerFacts(
    const std::string& epoch,
    std::uint64_t generation,
    std::uint64_t capabilityRevision)
{
    vdrsuite::agent::BackendAgentLocalProviderFacts facts;
    facts.providerId = vdrsuite::agent::kBackendAgentRecordingCutProviderId;
    facts.providerKind = vdrsuite::agent::kBackendAgentRecordingCutProviderKind;
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = generation;
    facts.capabilityRevision = capabilityRevision;
    facts.available = true;
    facts.capabilities = {vdrsuite::agent::kBackendAgentRecordingCutCapability};
    return facts;
}

void observeProvider(
    BackendAgentCommandRepository& commands,
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts,
    std::int64_t now)
{
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_cut";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {facts};
    const auto result = commands.poll(poll, "agt_cut", now);
    assert(result.accepted);
    assert(!result.assignment.present);
}

BackendAgentCommandPollResult pollCut(
    BackendAgentCommandRepository& commands,
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts,
    std::int64_t now)
{
    using namespace vdrsuite::agent;
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_cut";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = {kBackendAgentRecordingCutCommandType};
    poll.localProviders = {facts};
    return commands.poll(poll, "agt_cut", now);
}

vdrsuite::agent::BackendAgentRecordingCutAssignmentRequest request(
    const std::string& operationId,
    std::int64_t claimedAt)
{
    vdrsuite::agent::BackendAgentRecordingCutAssignmentRequest value;
    value.operationId = operationId;
    value.operationRevision = "rev-1";
    value.recordingKey = SourceRecordingKey;
    value.expectedMarksRevision = MarksRevision;
    value.backendId = "default";
    value.backendGeneration = 7;
    value.controlPlaneClaimedAt = claimedAt;
    return value;
}

BackendAgentCommandAssignment assign(
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    const std::string& operationId,
    std::int64_t claimedAt,
    std::int64_t now)
{
    vdrsuite::agent::BackendAgentRecordingCutAssignmentService service(
        commands, agents);
    const auto assigned = service.assign(
        systemContext(), request(operationId, claimedAt), now, now + 300);
    assert(assigned.accepted);
    assert(!assigned.replayed);
    return assigned.assignment;
}

void acceptReceipt(
    BackendAgentCommandRepository& commands,
    const BackendAgentCommandAssignment& assignment,
    std::int64_t receivedAt)
{
    BackendAgentCommandReceipt receipt;
    receipt.commandId = assignment.commandId;
    receipt.requestFingerprint = assignment.requestFingerprint;
    receipt.jobId = assignment.jobId;
    receipt.attemptId = assignment.attemptId;
    receipt.claimEpoch = assignment.claimEpoch;
    receipt.backendId = assignment.backendId;
    receipt.agentId = assignment.agentId;
    receipt.agentInstanceId = assignment.agentInstanceId;
    receipt.backendGeneration = assignment.backendGeneration;
    receipt.receiptCategory = "accepted";
    receipt.receivedAt = receivedAt;
    receipt.reasonCode = "recording_cut_received";
    const auto accepted = commands.acceptReceipt(receipt);
    assert(accepted.accepted);
}

void acceptAgentResult(
    BackendAgentCommandRepository& commands,
    const BackendAgentCommandAssignment& assignment,
    const std::string& dispatchState,
    std::int64_t completedAt,
    const std::string& editedRecordingKey = {})
{
    BackendAgentCommandResult result;
    result.commandId = assignment.commandId;
    result.requestFingerprint = assignment.requestFingerprint;
    result.jobId = assignment.jobId;
    result.attemptId = assignment.attemptId;
    result.claimEpoch = assignment.claimEpoch;
    result.backendId = assignment.backendId;
    result.agentId = assignment.agentId;
    result.agentInstanceId = assignment.agentInstanceId;
    result.backendGeneration = assignment.backendGeneration;
    result.dispatchState = dispatchState;
    result.verificationState = "outcome_unknown";
    result.resultCategory = "outcome_unknown";
    result.errorCategory = dispatchState == "accepted_by_executor"
        ? "none"
        : "executor_unknown";
    result.retryClassification = "reconcile_only";
    result.boundedDiagnostics = dispatchState == "accepted_by_executor"
        ? "recording cut accepted; native result reconciliation required"
        : "recording cut outcome unknown; reconciliation required";
    if (dispatchState == "accepted_by_executor" && !editedRecordingKey.empty())
    {
        result.boundedDiagnostics +=
            "; evidence=ncut:vdr:queued:" + editedRecordingKey + ':' +
            assignment.commandId;
    }
    result.completedAt = completedAt;
    const auto accepted = commands.acceptResult(result);
    assert(accepted.accepted);
}
}

int main()
{
    using namespace vdrsuite::agent;

    Database database;
    assert(database.open(":memory:"));
    BackendAgentRepository agents(database);
    BackendAgentCommandRepository commands(database);
    assert(agents.ensureSchema());
    assert(commands.ensureSchema());
    assert(commands.ensureRecordingCutReconciliationSchema());
    assert(database.execute(
        "INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,"
        "credential_id,credential_generation,agent_instance_id,"
        "backend_generation,protocol_version,software_version,"
        "heartbeat_sequence,capability_revision,last_connected_at,"
        "last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES("
        "'agt_cut','default','actor_cut','dev_cut','cred_cut',1,"
        "'inst_cut',7,'vdr-suite-agent/1','test',2,1,100,100,1000,1,1);"));

    observeProvider(commands, providerFacts("pie_cut_1", 3, 4), 101);
    BackendAgentLocalProviderOwnership ownership;
    std::string reasonCode;
    assert(commands.setLocalProviderOwnership(
        "default",
        kBackendAgentRecordingCutAuthorityDomain,
        kBackendAgentRecordingCutProviderId,
        kBackendAgentRecordingCutProviderKind,
        {kBackendAgentRecordingCutCapability},
        102,
        ownership,
        reasonCode));

    BackendAgentRecordingCutAssignmentService assignmentService(commands, agents);

    auto wrongGenerationRequest = request("op_cut_wrong_generation", 110);
    wrongGenerationRequest.backendGeneration = 8;
    const auto wrongGeneration = assignmentService.assign(
        systemContext(), wrongGenerationRequest, 111, 411);
    assert(!wrongGeneration.accepted);
    assert(wrongGeneration.reasonCode ==
        "recording_cut_backend_generation_conflict");

    assert(database.execute(
        "UPDATE backend_agents SET lease_expires_at=100,updated_at=112 "
        "WHERE agent_id='agt_cut';"));
    const auto expiredLease = assignmentService.assign(
        systemContext(), request("op_cut_expired_lease", 110), 113, 413);
    assert(!expiredLease.accepted);
    assert(expiredLease.reasonCode == "active_agent_lease_required");
    assert(database.execute(
        "UPDATE backend_agents SET lease_expires_at=1000,updated_at=114 "
        "WHERE agent_id='agt_cut';"));

    std::int64_t clock = 120;
    const auto accepted = assign(
        commands, agents, "op_cut_exact_result", clock, clock + 1);

    const auto exactReplay = assignmentService.assign(
        systemContext(), request("op_cut_exact_result", clock),
        clock + 2, clock + 302);
    assert(exactReplay.accepted);
    assert(exactReplay.replayed);
    assert(exactReplay.reasonCode == "recording_cut_assignment_replayed");
    assert(exactReplay.assignment.commandId == accepted.commandId);

    auto changedRevisionRequest = request("op_cut_exact_result", clock);
    changedRevisionRequest.expectedMarksRevision =
        "dddddddddddddddddddddddddddddddd";
    const auto changedRevision = assignmentService.assign(
        systemContext(), changedRevisionRequest, clock + 2, clock + 302);
    assert(!changedRevision.accepted);
    assert(changedRevision.reasonCode == "recording_cut_assignment_conflict");

    auto changedRecordingRequest = request("op_cut_exact_result", clock);
    changedRecordingRequest.recordingKey =
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    const auto changedRecording = assignmentService.assign(
        systemContext(), changedRecordingRequest, clock + 2, clock + 302);
    assert(!changedRecording.accepted);
    assert(changedRecording.reasonCode == "recording_cut_assignment_conflict");

    acceptReceipt(commands, accepted, clock + 2);
    acceptAgentResult(
        commands, accepted, "accepted_by_executor", clock + 3,
        EditedRecordingKey);

    auto candidates = commands.recordingCutReconciliationCandidates();
    assert(candidates.size() == 1);
    assert(candidates.front().assignment.commandId == accepted.commandId);
    assert(candidates.front().assignment.requestFingerprint ==
        accepted.requestFingerprint);
    assert(candidates.front().recordingKey == SourceRecordingKey);
    assert(candidates.front().expectedMarksRevision == MarksRevision);
    assert(candidates.front().editedRecordingKey == EditedRecordingKey);

    BackendAgentRecordingCutVerification verification;
    assert(!commands.verifyRecordingCutResult(
        accepted.commandId,
        accepted.requestFingerprint,
        SourceRecordingKey,
        MarksRevision,
        WrongEditedRecordingKey,
        clock + 4,
        verification,
        reasonCode));
    assert(reasonCode == "recording_cut_readback_state_mismatch");
    assert(!verification.present);
    assert(commands.recordingCutReconciliationCandidates().size() == 1);

    assert(commands.verifyRecordingCutResult(
        accepted.commandId,
        accepted.requestFingerprint,
        SourceRecordingKey,
        MarksRevision,
        EditedRecordingKey,
        clock + 4,
        verification,
        reasonCode));
    assert(reasonCode == "recording_cut_readback_verified");
    assert(verification.present);
    assert(verification.operationId == "op_cut_exact_result");
    assert(verification.recordingKey == SourceRecordingKey);
    assert(verification.expectedMarksRevision == MarksRevision);
    assert(verification.editedRecordingKey == EditedRecordingKey);

    const auto stored = commands.recordingCutVerificationForOperation(
        "default", "op_cut_exact_result");
    assert(stored.present);
    assert(stored.commandId == accepted.commandId);
    assert(stored.requestFingerprint == accepted.requestFingerprint);
    assert(stored.editedRecordingKey == EditedRecordingKey);
    assert(commands.recordingCutReconciliationCandidates().empty());

    assert(commands.verifyRecordingCutResult(
        accepted.commandId,
        accepted.requestFingerprint,
        SourceRecordingKey,
        MarksRevision,
        EditedRecordingKey,
        clock + 5,
        verification,
        reasonCode));
    assert(reasonCode == "recording_cut_readback_replayed");
    clock += 10;

    const auto malformedEvidence = assign(
        commands, agents, "op_cut_bad_evidence", clock, clock + 1);
    acceptReceipt(commands, malformedEvidence, clock + 2);
    acceptAgentResult(
        commands, malformedEvidence, "accepted_by_executor", clock + 3);
    assert(commands.recordingCutReconciliationCandidates().empty());
    assert(!commands.recordingCutVerificationForOperation(
        "default", "op_cut_bad_evidence").present);
    clock += 10;

    const auto unknown = assign(
        commands, agents, "op_cut_outcome_unknown", clock, clock + 1);
    acceptReceipt(commands, unknown, clock + 2);
    acceptAgentResult(commands, unknown, "starting", clock + 3);
    assert(commands.recordingCutReconciliationCandidates().empty());
    assert(!commands.recordingCutVerificationForOperation(
        "default", "op_cut_outcome_unknown").present);
    clock += 10;

    const auto staleProvider = assign(
        commands, agents, "op_cut_stale_provider", clock, clock + 1);
    acceptReceipt(commands, staleProvider, clock + 2);
    acceptAgentResult(
        commands,
        staleProvider,
        "accepted_by_executor",
        clock + 3,
        EditedRecordingKey);
    candidates = commands.recordingCutReconciliationCandidates();
    assert(candidates.size() == 1);
    assert(candidates.front().assignment.commandId == staleProvider.commandId);

    observeProvider(commands, providerFacts("pie_cut_2", 4, 5), clock + 4);
    assert(!commands.verifyRecordingCutResult(
        staleProvider.commandId,
        staleProvider.requestFingerprint,
        SourceRecordingKey,
        MarksRevision,
        EditedRecordingKey,
        clock + 5,
        verification,
        reasonCode));
    assert(!reasonCode.empty());
    assert(!commands.recordingCutVerificationForOperation(
        "default", "op_cut_stale_provider").present);

    const auto staleProviderReplay = assignmentService.assign(
        systemContext(), request("op_cut_stale_provider", clock),
        clock + 5, clock + 305);
    assert(!staleProviderReplay.accepted);
    assert(staleProviderReplay.reasonCode ==
        "recording_cut_provider_selection_stale");

    // Activation and delivery are exercised together, not merely assigned.
    clock += 20;
    const auto deliveryFacts = providerFacts("pie_cut_delivery_1", 4, 5);
    const auto initialPoll = pollCut(commands, deliveryFacts, clock);
    assert(initialPoll.accepted);
    assert(!initialPoll.assignment.present);
    assert(commands.hasCapability(
        "default", "agt_cut", "inst_cut", 7,
        kBackendAgentRecordingCutCommandType));
    assert(commands.ensureRecordingCutAssignmentSchema());
    assert(commands.hasCapability(
        "default", "agt_cut", "inst_cut", 7,
        kBackendAgentRecordingCutCommandType));
    const auto delivered = assign(
        commands, agents, "op_cut_delivery_regression", clock, clock + 1);
    const auto deliveredPoll = pollCut(commands, deliveryFacts, clock + 2);
    assert(deliveredPoll.accepted);
    assert(deliveredPoll.assignment.present);
    assert(deliveredPoll.assignment.commandId == delivered.commandId);
    assert(deliveredPoll.assignment.requestFingerprint ==
        delivered.requestFingerprint);
    const auto deliveredSummary = commands.summaryForBackend("default");
    assert(deliveredSummary.commandId == delivered.commandId);
    assert(deliveredSummary.deliveryCount == 1);
    const auto duplicatePoll = pollCut(commands, deliveryFacts, clock + 3);
    assert(duplicatePoll.accepted);
    assert(duplicatePoll.assignment.present);
    assert(duplicatePoll.assignment.commandId == delivered.commandId);
    assert(duplicatePoll.assignment.requestFingerprint ==
        delivered.requestFingerprint);
    assert(commands.summaryForBackend("default").deliveryCount == 2);
    acceptReceipt(commands, delivered, clock + 3);
    const auto acknowledgedPoll = pollCut(commands, deliveryFacts, clock + 3);
    assert(acknowledgedPoll.accepted);
    assert(!acknowledgedPoll.assignment.present);

    // A result is historical evidence: rotation after the acknowledged receipt
    // must not prevent persisting it or trigger another native execution.
    const auto rotatedFacts = providerFacts("pie_cut_delivery_2", 4, 5);
    assert(pollCut(commands, rotatedFacts, clock + 4).accepted);
    acceptAgentResult(
        commands, delivered, "accepted_by_executor", clock + 5,
        EditedRecordingKey);
    const auto storedResult = commands.summaryForBackend("default");
    assert(storedResult.commandId == delivered.commandId);
    assert(storedResult.deliveryCount == 2);

    // An undelivered assignment with a changed epoch stays fenced and expires.
    clock += 20;
    const auto staleAssignment = assign(
        commands, agents, "op_cut_delivery_stale", clock, clock + 1);
    const auto laterFacts = providerFacts("pie_cut_delivery_3", 4, 5);
    const auto stalePoll = pollCut(commands, laterFacts, clock + 2);
    assert(stalePoll.accepted);
    assert(!stalePoll.assignment.present);
    const auto staleSummary = commands.summaryForBackend("default");
    assert(staleSummary.commandId == staleAssignment.commandId);
    assert(staleSummary.deliveryCount == 0);
    const auto expiredPoll = pollCut(commands, laterFacts, clock + 302);
    assert(expiredPoll.accepted);
    assert(!expiredPoll.assignment.present);
    assert(commands.summaryForBackend("default").state == "expired");
    assert(commands.requestReplay("default", staleAssignment.commandId));
    assert(!pollCut(commands, laterFacts, clock + 303).assignment.present);
    const auto staleReplay = assignmentService.assign(
        systemContext(), request("op_cut_delivery_stale", clock),
        clock + 304, clock + 604);
    assert(!staleReplay.accepted);
    assert(staleReplay.reasonCode == "recording_cut_provider_selection_stale");
    assert(commands.summaryForBackend("default").deliveryCount == 0);

    // A new, separately authorized operation can be delivered on the new fence.
    const auto next = assign(
        commands, agents, "op_cut_delivery_new", clock + 305, clock + 306);
    const auto nextPoll = pollCut(commands, laterFacts, clock + 307);
    assert(nextPoll.accepted);
    assert(nextPoll.assignment.present);
    assert(nextPoll.assignment.commandId == next.commandId);
    assert(nextPoll.assignment.commandId != staleAssignment.commandId);

    return 0;
}

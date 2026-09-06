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

    std::int64_t clock = 110;
    const auto accepted = assign(
        commands, agents, "op_cut_exact_result", clock, clock + 1);
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

    return 0;
}

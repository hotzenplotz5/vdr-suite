#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingMarksModifyAssignment.h"
#include "Database.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
RequestSecurityContext systemContext()
{
    RequestSecurityContext value;
    value.requestId = "req_marks_reconciliation";
    value.correlationId = "corr_marks_reconciliation";
    value.authenticationState = AuthenticationState::Authenticated;
    value.actor = ActorIdentity{
        "system_marks_reconciliation", ActorType::System, "test", true};
    value.device = DeviceIdentity{"dev_marks_reconciliation", true};
    value.credential = CredentialIdentity{
        "cred_marks_reconciliation", true, false, false};
    value.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return value;
}

vdrsuite::agent::BackendAgentLocalProviderFacts providerFacts(
    const std::string& epoch,
    std::uint64_t generation,
    std::uint64_t capabilityRevision)
{
    vdrsuite::agent::BackendAgentLocalProviderFacts facts;
    facts.providerId =
        vdrsuite::agent::kBackendAgentRecordingMarksModifyProviderId;
    facts.providerKind =
        vdrsuite::agent::kBackendAgentRecordingMarksModifyProviderKind;
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = generation;
    facts.capabilityRevision = capabilityRevision;
    facts.available = true;
    facts.capabilities = {
        vdrsuite::agent::kBackendAgentRecordingMarksModifyCapability};
    return facts;
}

void observeProvider(
    BackendAgentCommandRepository& commands,
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts,
    std::int64_t now)
{
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_marks";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {facts};
    const auto result = commands.poll(poll, "agt_marks", now);
    assert(result.accepted);
    assert(!result.assignment.present);
}

vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentRequest request(
    const std::string& operationId,
    std::int64_t claimedAt)
{
    vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentRequest value;
    value.kind = vdrsuite::agent::BackendAgentRecordingMarksModifyKind::add;
    value.operationId = operationId;
    value.operationRevision = "rev-1";
    value.recordingKey = "0123456789abcdef0123456789abcdef";
    value.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    value.targetFrame = 500;
    value.backendId = "default";
    value.backendGeneration = 7;
    value.controlPlaneClaimedAt = claimedAt;
    return value;
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
    receipt.reasonCode = "recording_marks_modify_received";
    const auto accepted = commands.acceptReceipt(receipt);
    assert(accepted.accepted);
}

void acceptAgentResult(
    BackendAgentCommandRepository& commands,
    const BackendAgentCommandAssignment& assignment,
    const std::string& dispatchState,
    std::int64_t completedAt)
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
        ? "recording marks modify accepted; readback reconciliation required"
        : "recording marks modify outcome unknown; reconciliation required";
    result.completedAt = completedAt;
    const auto accepted = commands.acceptResult(result);
    assert(accepted.accepted);
}

BackendAgentCommandAssignment assign(
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    const std::string& operationId,
    std::int64_t claimedAt,
    std::int64_t now)
{
    vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentService service(
        commands, agents);
    const auto assigned = service.assign(
        systemContext(), request(operationId, claimedAt), now, now + 300);
    assert(assigned.accepted);
    assert(!assigned.replayed);
    return assigned.assignment;
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
    assert(commands.ensureRecordingMarksModifyReconciliationSchema());
    assert(database.execute(
        "INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,"
        "credential_id,credential_generation,agent_instance_id,"
        "backend_generation,protocol_version,software_version,"
        "heartbeat_sequence,capability_revision,last_connected_at,"
        "last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES("
        "'agt_marks','default','actor_marks','dev_marks','cred_marks',1,"
        "'inst_marks',7,'vdr-suite-agent/1','test',2,1,100,100,1000,1,1);"));

    observeProvider(commands, providerFacts("pie_marks_1", 3, 4), 101);
    BackendAgentLocalProviderOwnership ownership;
    std::string reasonCode;
    assert(commands.setLocalProviderOwnership(
        "default",
        kBackendAgentRecordingMarksModifyAuthorityDomain,
        kBackendAgentRecordingMarksModifyProviderId,
        kBackendAgentRecordingMarksModifyProviderKind,
        {kBackendAgentRecordingMarksModifyCapability},
        102,
        ownership,
        reasonCode));

    const auto accepted = assign(
        commands, agents, "op_marks_readback", 110, 120);
    acceptReceipt(commands, accepted, 121);
    acceptAgentResult(commands, accepted, "accepted_by_executor", 130);

    auto candidates = commands.recordingMarksModifyReconciliationCandidates();
    assert(candidates.size() == 1);
    assert(candidates.front().assignment.commandId == accepted.commandId);
    assert(candidates.front().assignment.requestFingerprint ==
        accepted.requestFingerprint);
    assert(candidates.front().recordingKey ==
        "0123456789abcdef0123456789abcdef");
    assert(candidates.front().expectedMarksRevision ==
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    assert(candidates.front().executorCompletedAt == 130);

    BackendAgentRecordingMarksModifyVerification verification;
    assert(!commands.verifyRecordingMarksModifyReadback(
        accepted.commandId,
        accepted.requestFingerprint,
        candidates.front().recordingKey,
        candidates.front().expectedMarksRevision,
        candidates.front().expectedMarksRevision,
        131,
        verification,
        reasonCode));
    assert(reasonCode == "recording_marks_modify_readback_invalid");

    assert(!commands.verifyRecordingMarksModifyReadback(
        accepted.commandId,
        accepted.requestFingerprint,
        candidates.front().recordingKey,
        candidates.front().expectedMarksRevision,
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        129,
        verification,
        reasonCode));
    assert(reasonCode == "recording_marks_modify_readback_candidate_conflict");

    assert(commands.verifyRecordingMarksModifyReadback(
        accepted.commandId,
        accepted.requestFingerprint,
        candidates.front().recordingKey,
        candidates.front().expectedMarksRevision,
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        131,
        verification,
        reasonCode));
    assert(reasonCode == "recording_marks_modify_readback_verified");
    assert(verification.present);
    assert(verification.operationId == "op_marks_readback");
    assert(verification.canonicalMarksRevision ==
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

    const auto stored = commands.recordingMarksModifyVerificationForOperation(
        "default", "op_marks_readback");
    assert(stored.present);
    assert(stored.commandId == accepted.commandId);
    assert(stored.requestFingerprint == accepted.requestFingerprint);
    assert(stored.recordingKey == candidates.front().recordingKey);
    assert(stored.expectedMarksRevision ==
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    assert(stored.canonicalMarksRevision ==
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    assert(stored.verifiedAt == 131);
    assert(commands.recordingMarksModifyReconciliationCandidates().empty());

    assert(commands.verifyRecordingMarksModifyReadback(
        accepted.commandId,
        accepted.requestFingerprint,
        stored.recordingKey,
        stored.expectedMarksRevision,
        stored.canonicalMarksRevision,
        132,
        verification,
        reasonCode));
    assert(reasonCode == "recording_marks_modify_readback_replayed");

    assert(!commands.verifyRecordingMarksModifyReadback(
        accepted.commandId,
        accepted.requestFingerprint,
        stored.recordingKey,
        stored.expectedMarksRevision,
        "cccccccccccccccccccccccccccccccc",
        132,
        verification,
        reasonCode));
    assert(reasonCode == "recording_marks_modify_readback_conflict");

    const auto unknown = assign(
        commands, agents, "op_marks_outcome_unknown", 140, 141);
    acceptReceipt(commands, unknown, 142);
    acceptAgentResult(commands, unknown, "starting", 150);
    assert(commands.recordingMarksModifyReconciliationCandidates().empty());
    assert(!commands.recordingMarksModifyVerificationForOperation(
        "default", "op_marks_outcome_unknown").present);

    const auto staleProvider = assign(
        commands, agents, "op_marks_stale_provider", 151, 152);
    acceptReceipt(commands, staleProvider, 153);
    acceptAgentResult(commands, staleProvider, "accepted_by_executor", 160);
    candidates = commands.recordingMarksModifyReconciliationCandidates();
    assert(candidates.size() == 1);
    assert(candidates.front().assignment.commandId == staleProvider.commandId);

    observeProvider(commands, providerFacts("pie_marks_2", 4, 5), 161);
    assert(!commands.verifyRecordingMarksModifyReadback(
        staleProvider.commandId,
        staleProvider.requestFingerprint,
        candidates.front().recordingKey,
        candidates.front().expectedMarksRevision,
        "dddddddddddddddddddddddddddddddd",
        162,
        verification,
        reasonCode));
    assert(!reasonCode.empty());
    assert(!commands.recordingMarksModifyVerificationForOperation(
        "default", "op_marks_stale_provider").present);

    return 0;
}

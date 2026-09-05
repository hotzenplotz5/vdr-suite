#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingMarksModifyAssignment.h"
#include "Database.h"

#include <cassert>
#include <string>
#include <utility>
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
    std::int64_t claimedAt,
    vdrsuite::agent::BackendAgentRecordingMarksModifyKind kind =
        vdrsuite::agent::BackendAgentRecordingMarksModifyKind::add)
{
    using vdrsuite::agent::BackendAgentRecordingMarksModifyKind;
    vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentRequest value;
    value.kind = kind;
    value.operationId = operationId;
    value.operationRevision = "rev-1";
    value.recordingKey = "0123456789abcdef0123456789abcdef";
    value.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    if (kind == BackendAgentRecordingMarksModifyKind::add)
        value.targetFrame = 500;
    else if (kind == BackendAgentRecordingMarksModifyKind::deleteMark)
        value.sourceFrame = 250;
    else if (kind == BackendAgentRecordingMarksModifyKind::move)
    {
        value.sourceFrame = 250;
        value.targetFrame = 500;
    }
    else if (kind == BackendAgentRecordingMarksModifyKind::replace)
        value.replacementFrames = {250, 500};
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
    std::int64_t completedAt,
    const std::string& postRevision = {})
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
    if (dispatchState == "accepted_by_executor" && !postRevision.empty())
    {
        result.boundedDiagnostics +=
            "; evidence=nmarks:vdr:postrev:" + postRevision + ':' +
            assignment.commandId;
    }
    result.completedAt = completedAt;
    const auto accepted = commands.acceptResult(result);
    assert(accepted.accepted);
}

BackendAgentCommandAssignment assign(
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    const std::string& operationId,
    std::int64_t claimedAt,
    std::int64_t now,
    vdrsuite::agent::BackendAgentRecordingMarksModifyKind kind =
        vdrsuite::agent::BackendAgentRecordingMarksModifyKind::add)
{
    vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentService service(
        commands, agents);
    const auto assigned = service.assign(
        systemContext(), request(operationId, claimedAt, kind), now, now + 300);
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

    struct Case
    {
        const char* operationId;
        BackendAgentRecordingMarksModifyKind kind;
        const char* postRevision;
    };
    const std::vector<Case> cases = {
        {"op_marks_add", BackendAgentRecordingMarksModifyKind::add,
         "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"},
        {"op_marks_delete", BackendAgentRecordingMarksModifyKind::deleteMark,
         "cccccccccccccccccccccccccccccccc"},
        {"op_marks_move", BackendAgentRecordingMarksModifyKind::move,
         "dddddddddddddddddddddddddddddddd"},
        {"op_marks_reset", BackendAgentRecordingMarksModifyKind::reset,
         "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"},
        {"op_marks_replace", BackendAgentRecordingMarksModifyKind::replace,
         "ffffffffffffffffffffffffffffffff"},
    };

    std::int64_t clock = 110;
    for (const Case& test : cases)
    {
        const auto accepted = assign(
            commands, agents, test.operationId, clock, clock + 1, test.kind);
        acceptReceipt(commands, accepted, clock + 2);
        acceptAgentResult(
            commands, accepted, "accepted_by_executor", clock + 3,
            test.postRevision);

        auto candidates = commands.recordingMarksModifyReconciliationCandidates();
        assert(candidates.size() == 1);
        assert(candidates.front().assignment.commandId == accepted.commandId);
        assert(candidates.front().assignment.requestFingerprint ==
            accepted.requestFingerprint);
        assert(candidates.front().recordingKey ==
            "0123456789abcdef0123456789abcdef");
        assert(candidates.front().expectedMarksRevision ==
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

        BackendAgentRecordingMarksModifyVerification verification;
        assert(!commands.verifyRecordingMarksModifyReadback(
            accepted.commandId,
            accepted.requestFingerprint,
            candidates.front().recordingKey,
            candidates.front().expectedMarksRevision,
            "11111111111111111111111111111111",
            clock + 4,
            verification,
            reasonCode));
        assert(reasonCode == "recording_marks_modify_readback_state_mismatch");
        assert(!verification.present);
        assert(commands.recordingMarksModifyReconciliationCandidates().size() == 1);

        assert(commands.verifyRecordingMarksModifyReadback(
            accepted.commandId,
            accepted.requestFingerprint,
            candidates.front().recordingKey,
            candidates.front().expectedMarksRevision,
            test.postRevision,
            clock + 4,
            verification,
            reasonCode));
        assert(reasonCode == "recording_marks_modify_readback_verified");
        assert(verification.present);
        assert(verification.operationId == test.operationId);
        assert(verification.canonicalMarksRevision == test.postRevision);

        const auto stored = commands.recordingMarksModifyVerificationForOperation(
            "default", test.operationId);
        assert(stored.present);
        assert(stored.commandId == accepted.commandId);
        assert(stored.requestFingerprint == accepted.requestFingerprint);
        assert(stored.canonicalMarksRevision == test.postRevision);
        assert(commands.recordingMarksModifyReconciliationCandidates().empty());

        assert(commands.verifyRecordingMarksModifyReadback(
            accepted.commandId,
            accepted.requestFingerprint,
            stored.recordingKey,
            stored.expectedMarksRevision,
            stored.canonicalMarksRevision,
            clock + 5,
            verification,
            reasonCode));
        assert(reasonCode == "recording_marks_modify_readback_replayed");
        clock += 10;
    }

    const auto malformedEvidence = assign(
        commands, agents, "op_marks_bad_evidence", clock, clock + 1);
    acceptReceipt(commands, malformedEvidence, clock + 2);
    acceptAgentResult(
        commands, malformedEvidence, "accepted_by_executor", clock + 3);
    assert(commands.recordingMarksModifyReconciliationCandidates().empty());
    assert(!commands.recordingMarksModifyVerificationForOperation(
        "default", "op_marks_bad_evidence").present);
    clock += 10;

    const auto unknown = assign(
        commands, agents, "op_marks_outcome_unknown", clock, clock + 1);
    acceptReceipt(commands, unknown, clock + 2);
    acceptAgentResult(commands, unknown, "starting", clock + 3);
    assert(commands.recordingMarksModifyReconciliationCandidates().empty());
    assert(!commands.recordingMarksModifyVerificationForOperation(
        "default", "op_marks_outcome_unknown").present);
    clock += 10;

    const auto staleProvider = assign(
        commands, agents, "op_marks_stale_provider", clock, clock + 1);
    acceptReceipt(commands, staleProvider, clock + 2);
    acceptAgentResult(
        commands,
        staleProvider,
        "accepted_by_executor",
        clock + 3,
        "99999999999999999999999999999999");
    auto candidates = commands.recordingMarksModifyReconciliationCandidates();
    assert(candidates.size() == 1);
    assert(candidates.front().assignment.commandId == staleProvider.commandId);

    observeProvider(commands, providerFacts("pie_marks_2", 4, 5), clock + 4);
    BackendAgentRecordingMarksModifyVerification verification;
    assert(!commands.verifyRecordingMarksModifyReadback(
        staleProvider.commandId,
        staleProvider.requestFingerprint,
        candidates.front().recordingKey,
        candidates.front().expectedMarksRevision,
        "99999999999999999999999999999999",
        clock + 5,
        verification,
        reasonCode));
    assert(!reasonCode.empty());
    assert(!commands.recordingMarksModifyVerificationForOperation(
        "default", "op_marks_stale_provider").present);

    return 0;
}

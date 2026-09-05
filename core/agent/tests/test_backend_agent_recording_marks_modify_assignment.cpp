#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingMarksModifyAssignment.h"
#include "BackendAgentRecordingMarksModifyPayload.h"
#include "Database.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{

RequestSecurityContext context(ActorType type, const std::string& actorId)
{
    RequestSecurityContext value;
    value.requestId = "req_marks_assignment_test";
    value.correlationId = "corr_marks_assignment_test";
    value.authenticationState = AuthenticationState::Authenticated;
    value.actor = ActorIdentity{actorId, type, "test", true};
    value.device = DeviceIdentity{"dev_marks_assignment", true};
    value.credential = CredentialIdentity{
        "cred_marks_assignment", true, false, false};
    value.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return value;
}

vdrsuite::agent::BackendAgentLocalProviderFacts providerFacts(
    const std::string& epoch = "pie_marks_1",
    std::uint64_t generation = 3,
    std::uint64_t capabilityRevision = 4,
    bool includeMarksModify = true)
{
    vdrsuite::agent::BackendAgentLocalProviderFacts facts;
    facts.providerId =
        vdrsuite::agent::kBackendAgentRecordingMarksModifyProviderId;
    facts.providerKind = "suitebridge";
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = generation;
    facts.capabilityRevision = capabilityRevision;
    facts.available = true;
    facts.capabilities = includeMarksModify
        ? std::vector<std::string>{"vdr.recording.marks.modify"}
        : std::vector<std::string>{"vdr.native.probe"};
    return facts;
}

void observeProvider(
    BackendAgentCommandRepository& commands,
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts,
    std::int64_t now,
    const std::string& agentInstanceId = "inst_marks")
{
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = agentInstanceId;
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {facts};
    const auto result = commands.poll(poll, "agt_marks", now);
    assert(result.accepted);
    assert(!result.assignment.present);
}

vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentRequest request(
    const std::string& operationId = "op_marks_1")
{
    vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentRequest value;
    value.kind = vdrsuite::agent::BackendAgentRecordingMarksModifyKind::move;
    value.operationId = operationId;
    value.operationRevision = "3";
    value.recordingKey = "0123456789abcdef0123456789abcdef";
    value.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    value.sourceFrame = 100;
    value.targetFrame = 120;
    value.backendId = "default";
    value.backendGeneration = 7;
    value.controlPlaneClaimedAt = 110;
    return value;
}

vdrsuite::agent::BackendAgentRecordingMarksModifyCommand commandFrom(
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentRecordingMarksModifyPayload& payload)
{
    vdrsuite::agent::BackendAgentRecordingMarksModifyCommand command;
    command.kind = payload.kind;
    command.commandId = assignment.commandId;
    command.requestFingerprint = assignment.requestFingerprint;
    command.operationId = assignment.operationId;
    command.operationRevision = payload.operationRevision;
    command.recordingKey = payload.recordingKey;
    command.expectedMarksRevision = payload.expectedMarksRevision;
    command.sourceFrame = payload.sourceFrame;
    command.targetFrame = payload.targetFrame;
    command.replacementFrames = payload.replacementFrames;
    command.jobId = assignment.jobId;
    command.attemptId = assignment.attemptId;
    command.claimEpoch = assignment.claimEpoch;
    command.backendId = assignment.backendId;
    command.agentId = assignment.agentId;
    command.agentInstanceId = assignment.agentInstanceId;
    command.backendGeneration = assignment.backendGeneration;
    command.controlPlaneClaimedAt = payload.controlPlaneClaimedAt;
    command.localProviderSelection = payload.localProviderSelection;
    return command;
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
    assert(database.execute(
        "INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,"
        "credential_id,credential_generation,agent_instance_id,"
        "backend_generation,protocol_version,software_version,"
        "heartbeat_sequence,capability_revision,last_connected_at,"
        "last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES("
        "'agt_marks','default','actor_marks','dev_marks','cred_marks',1,"
        "'inst_marks',7,'vdr-suite-agent/1','test',2,1,100,100,1000,1,1);"));

    BackendAgentRecordingMarksModifyAssignmentService service(commands, agents);
    const auto system = context(ActorType::System, "system_marks");
    const auto agentActor = context(ActorType::Agent, "actor_marks");

    observeProvider(commands, providerFacts(), 101);

    const auto noOwnership = service.assign(system, request(), 120, 500);
    assert(!noOwnership.accepted);
    assert(noOwnership.reasonCode == "local_provider_ownership_required");

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
    assert(ownership.ownershipGeneration == 1);

    const auto assigned = service.assign(system, request(), 120, 500);
    assert(assigned.accepted);
    assert(!assigned.replayed);
    assert(assigned.reasonCode == "recording_marks_modify_assigned");
    assert(assigned.assignment.commandType ==
        kBackendAgentRecordingMarksModifyCommandType);
    assert(assigned.assignment.payloadVersion == 1);
    assert(assigned.assignment.verificationPolicy == "readback_required");
    assert(assigned.assignment.operationId == "op_marks_1");
    assert(assigned.assignment.backendGeneration == 7);
    assert(assigned.assignment.requestFingerprint ==
        backendAgentCommandFingerprint(assigned.assignment));
    assert(backendAgentCommandValidAssignment(assigned.assignment));
    assert(!commands.hasCapability(
        "default",
        "agt_marks",
        "inst_marks",
        7,
        kBackendAgentRecordingMarksModifyCommandType));

    BackendAgentRecordingMarksModifyPayload payload;
    assert(backendAgentRecordingMarksModifyParsePayload(
        assigned.assignment.payload, payload, reasonCode));
    assert(payload.operationRevision == "3");
    assert(payload.recordingKey == "0123456789abcdef0123456789abcdef");
    assert(payload.expectedMarksRevision ==
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    assert(payload.kind == BackendAgentRecordingMarksModifyKind::move);
    assert(payload.sourceFrame == 100);
    assert(payload.targetFrame == 120);
    assert(payload.localProviderSelection.authorityDomain ==
        kBackendAgentRecordingMarksModifyAuthorityDomain);
    assert(payload.localProviderSelection.requiredCapability ==
        kBackendAgentRecordingMarksModifyCapability);
    assert(payload.localProviderSelection.providerInstanceEpoch == "pie_marks_1");
    assert(payload.localProviderSelection.providerGeneration == 3);
    assert(payload.localProviderSelection.capabilityRevision == 4);

    const auto sidecar = commands.localProviderSelectionForCommand(
        assigned.assignment.commandId);
    assert(sidecar.has_value());
    assert(backendAgentLocalProviderSameFence(
        *sidecar, payload.localProviderSelection));
    assert(backendAgentRecordingMarksModifyValidCommand(
        commandFrom(assigned.assignment, payload), reasonCode));

    const auto replay = service.assign(system, request(), 121, 501);
    assert(replay.accepted);
    assert(replay.replayed);
    assert(replay.reasonCode == "recording_marks_modify_assignment_replayed");
    assert(replay.assignment.commandId == assigned.assignment.commandId);
    assert(replay.assignment.jobId == assigned.assignment.jobId);

    auto changedRevisionRequest = request();
    changedRevisionRequest.expectedMarksRevision =
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    const auto changedRevision = service.assign(
        system, changedRevisionRequest, 122, 502);
    assert(!changedRevision.accepted);
    assert(changedRevision.reasonCode ==
        "recording_marks_modify_assignment_conflict");

    auto changedRecordingRequest = request();
    changedRecordingRequest.recordingKey =
        "fedcba9876543210fedcba9876543210";
    const auto changedRecording = service.assign(
        system, changedRecordingRequest, 122, 502);
    assert(!changedRecording.accepted);
    assert(changedRecording.reasonCode ==
        "recording_marks_modify_assignment_conflict");

    auto changedMutationRequest = request();
    changedMutationRequest.kind = BackendAgentRecordingMarksModifyKind::add;
    changedMutationRequest.sourceFrame = -1;
    changedMutationRequest.targetFrame = 130;
    const auto changedMutation = service.assign(
        system, changedMutationRequest, 122, 502);
    assert(!changedMutation.accepted);
    assert(changedMutation.reasonCode ==
        "recording_marks_modify_assignment_conflict");

    auto wrongGenerationRequest = request("op_marks_generation");
    wrongGenerationRequest.backendGeneration = 8;
    const auto wrongGeneration = service.assign(
        system, wrongGenerationRequest, 123, 503);
    assert(!wrongGeneration.accepted);
    assert(wrongGeneration.reasonCode ==
        "recording_marks_modify_backend_generation_conflict");

    const auto wrongActor = service.assign(
        agentActor, request("op_marks_actor"), 124, 504);
    assert(!wrongActor.accepted);
    assert(wrongActor.reasonCode ==
        "invalid_recording_marks_modify_assignment_request");

    auto futureClaim = request("op_marks_future");
    futureClaim.controlPlaneClaimedAt = 200;
    const auto invalidTime = service.assign(system, futureClaim, 125, 505);
    assert(!invalidTime.accepted);
    assert(invalidTime.reasonCode ==
        "invalid_recording_marks_modify_assignment_request");

    observeProvider(commands, providerFacts("pie_marks_2", 4, 5), 130);
    const auto staleReplay = service.assign(system, request(), 131, 511);
    assert(!staleReplay.accepted);
    assert(staleReplay.reasonCode ==
        "recording_marks_modify_provider_selection_stale");

    const auto next = service.assign(
        system, request("op_marks_2"), 132, 512);
    assert(next.accepted);
    assert(!next.replayed);
    assert(next.assignment.commandId != assigned.assignment.commandId);

    observeProvider(
        commands, providerFacts("pie_marks_3", 5, 6, false), 140);
    const auto noObservedCapability = service.assign(
        system, request("op_marks_3"), 141, 521);
    assert(!noObservedCapability.accepted);
    assert(noObservedCapability.reasonCode ==
        "local_provider_capability_not_observed");

    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_marks";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {
        providerFacts("pie_marks_3", 5, 6, false)};
    const auto notDelivered = commands.poll(poll, "agt_marks", 142);
    assert(notDelivered.accepted);
    assert(!notDelivered.assignment.present);

    assert(database.execute(
        "UPDATE backend_agents SET agent_instance_id='inst_marks_2',"
        "lease_expires_at=1000,updated_at=150 WHERE agent_id='agt_marks';"));
    observeProvider(
        commands,
        providerFacts("pie_marks_4", 6, 7),
        150,
        "inst_marks_2");
    const auto staleAgentReplay = service.assign(system, request(), 151, 531);
    assert(!staleAgentReplay.accepted);
    assert(staleAgentReplay.reasonCode ==
        "recording_marks_modify_agent_fence_stale");

    return 0;
}

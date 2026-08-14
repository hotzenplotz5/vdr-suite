#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerDelete.h"
#include "BackendAgentNativeTimerDeleteAssignment.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"
#include "BackendAgentNativeTimerDeletePayload.h"
#include "Database.h"

#include <cassert>
#include <string>

namespace
{
RequestSecurityContext context(ActorType type, const std::string& actor)
{
    RequestSecurityContext value;
    value.requestId = "req_timer_delete_assignment_test";
    value.correlationId = "corr_timer_delete_assignment_test";
    value.authenticationState = AuthenticationState::Authenticated;
    value.actor = ActorIdentity{actor, type, "test", true};
    value.device = DeviceIdentity{"dev_timer_delete_assignment", true};
    value.credential = CredentialIdentity{
        "cred_timer_delete_assignment", true, false, false};
    value.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return value;
}

std::string canonicalObservedFingerprint(const std::string& marker)
{
    return "native-timer-observed-state/1|9:channel:1|11:Movie \"A " +
        marker + "\"|" + std::string(300, 'x') + "|";
}

vdrsuite::agent::BackendAgentLocalProviderFacts providerFacts(
    const std::string& epoch = "pie_timer_delete_1",
    std::uint64_t generation = 3,
    std::uint64_t capabilityRevision = 4,
    bool includeDelete = true)
{
    vdrsuite::agent::BackendAgentLocalProviderFacts facts;
    facts.providerId = "suitebridge:local";
    facts.providerKind = "suitebridge";
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = generation;
    facts.capabilityRevision = capabilityRevision;
    facts.available = true;
    facts.capabilities = includeDelete
        ? std::vector<std::string>{"vdr.timer.delete"}
        : std::vector<std::string>{"vdr.native.probe"};
    return facts;
}

void observeProvider(
    BackendAgentCommandRepository& commands,
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts,
    std::int64_t now,
    const std::string& agentInstanceId = "inst_timer_delete")
{
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = agentInstanceId;
    poll.backendGeneration = 7;
    // Deliberately do not advertise vdr.timer.delete as an executable Agent
    // command in Slice 25. Only the provider fact is observed.
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {facts};
    const auto result = commands.poll(poll, "agt_timer_delete", now);
    assert(result.accepted);
    assert(!result.assignment.present);
}

vdrsuite::agent::BackendAgentNativeTimerDeleteAssignmentRequest request(
    const std::string& operationId = "op_timer_delete_1")
{
    vdrsuite::agent::BackendAgentNativeTimerDeleteAssignmentRequest value;
    value.operationId = operationId;
    value.operationRevision = "3";
    value.nativeTimerBindingId = "ntb_timer_1";
    value.expectedBindingRevision = "12";
    value.expectedNativeTimerFingerprint = canonicalObservedFingerprint("44");
    value.timerAssignmentId = "ta_timer_1";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.backendNativeTimerId = "native_timer_44";
    value.controlPlaneClaimedAt = 110;
    return value;
}

vdrsuite::agent::BackendAgentNativeTimerDeleteCommand commandFrom(
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentNativeTimerDeletePayload& payload)
{
    vdrsuite::agent::BackendAgentNativeTimerDeleteCommand command;
    command.commandId = assignment.commandId;
    command.requestFingerprint = assignment.requestFingerprint;
    command.operationId = assignment.operationId;
    command.operationRevision = payload.operationRevision;
    command.nativeTimerBindingId = payload.nativeTimerBindingId;
    command.expectedBindingRevision = payload.expectedBindingRevision;
    command.expectedNativeTimerFingerprint = payload.expectedNativeTimerFingerprint;
    command.timerAssignmentId = payload.timerAssignmentId;
    command.backendNativeTimerId = payload.backendNativeTimerId;
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
        "'agt_timer_delete','default','actor_timer_delete','dev_timer_delete',"
        "'cred_timer_delete',1,'inst_timer_delete',7,'vdr-suite-agent/1',"
        "'test',2,1,100,100,1000,1,1);"));

    BackendAgentNativeTimerDeleteAssignmentService service(commands, agents);
    const auto system = context(ActorType::System, "system_timer_delete");
    const auto agentActor = context(ActorType::Agent, "actor_timer_delete");

    observeProvider(commands, providerFacts(), 101);

    auto noOwnership = service.assign(system, request(), 120, 500);
    assert(!noOwnership.accepted);
    assert(noOwnership.reasonCode == "local_provider_ownership_required");

    BackendAgentLocalProviderOwnership ownership;
    std::string reason;
    assert(commands.setLocalProviderOwnership(
        "default", "vdr.timer", "suitebridge:local", "suitebridge",
        {"vdr.timer.delete"}, 102, ownership, reason));
    assert(ownership.ownershipGeneration == 1);

    const auto assigned = service.assign(system, request(), 120, 500);
    assert(assigned.accepted);
    assert(!assigned.replayed);
    assert(assigned.reasonCode == "native_timer_delete_assigned");
    assert(assigned.assignment.commandType == "vdr.timer.delete");
    assert(assigned.assignment.payloadVersion == 1);
    assert(assigned.assignment.verificationPolicy == "readback_required");
    assert(assigned.assignment.operationId == "op_timer_delete_1");
    assert(assigned.assignment.backendGeneration == 7);
    assert(assigned.assignment.requestFingerprint ==
        backendAgentCommandFingerprint(assigned.assignment));
    assert(backendAgentCommandValidAssignment(assigned.assignment));
    assert(!commands.hasCapability(
        "default", "agt_timer_delete", "inst_timer_delete", 7,
        "vdr.timer.delete"));

    // Slice 25's assignment helper created the temporary dormant capability
    // gate. Slice 26 deliberately retires that gate when the exact bounded
    // Timer-delete command/provider advertisement reaches command polling.
    BackendAgentCommandPollRequest claimedDeleteCapability;
    claimedDeleteCapability.backendId = "default";
    claimedDeleteCapability.agentInstanceId = "inst_timer_delete";
    claimedDeleteCapability.backendGeneration = 7;
    claimedDeleteCapability.supportedCommandTypes = {"vdr.timer.delete"};
    claimedDeleteCapability.localProviders = {providerFacts()};
    const auto blockedDelivery = commands.poll(
        claimedDeleteCapability, "agt_timer_delete", 120);
    assert(blockedDelivery.accepted);
    assert(blockedDelivery.assignment.present);
    assert(blockedDelivery.assignment.commandId == assigned.assignment.commandId);
    assert(commands.hasCapability(
        "default", "agt_timer_delete", "inst_timer_delete", 7,
        "vdr.timer.delete"));

    const std::string rawExpectedFingerprint =
        request().expectedNativeTimerFingerprint;
    const std::string expectedFingerprintToken =
        backendAgentNativeTimerDeleteFingerprintToken(rawExpectedFingerprint);
    assert(rawExpectedFingerprint.size() > 256);
    assert(rawExpectedFingerprint.find(' ') != std::string::npos);
    assert(rawExpectedFingerprint.find('|') != std::string::npos);
    assert(expectedFingerprintToken.size() > 512);
    assert(backendAgentNativeTimerDeleteFingerprintTokenValid(
        expectedFingerprintToken));

    BackendAgentNativeTimerDeletePayload payload;
    assert(backendAgentNativeTimerDeleteParsePayload(
        assigned.assignment.payload, payload, reason));
    assert(payload.operationRevision == "3");
    assert(payload.nativeTimerBindingId == "ntb_timer_1");
    assert(payload.expectedBindingRevision == "12");
    assert(payload.expectedNativeTimerFingerprint == expectedFingerprintToken);
    assert(payload.expectedNativeTimerFingerprint != rawExpectedFingerprint);
    assert(payload.timerAssignmentId == "ta_timer_1");
    assert(payload.backendNativeTimerId == "native_timer_44");
    assert(payload.controlPlaneClaimedAt == 110);
    assert(payload.localProviderSelection.authorityDomain == "vdr.timer");
    assert(payload.localProviderSelection.requiredCapability == "vdr.timer.delete");
    assert(payload.localProviderSelection.providerInstanceEpoch ==
           "pie_timer_delete_1");
    assert(payload.localProviderSelection.providerGeneration == 3);
    assert(payload.localProviderSelection.capabilityRevision == 4);

    const auto sidecar =
        commands.localProviderSelectionForCommand(assigned.assignment.commandId);
    assert(sidecar.has_value());
    assert(backendAgentLocalProviderSameFence(
        *sidecar, payload.localProviderSelection));

    const auto domain = commandFrom(assigned.assignment, payload);
    assert(domain.expectedNativeTimerFingerprint == expectedFingerprintToken);
    assert(backendAgentNativeTimerDeleteValidCommand(domain, reason));

    const auto replay = service.assign(system, request(), 121, 501);
    assert(replay.accepted);
    assert(replay.replayed);
    assert(replay.reasonCode == "native_timer_delete_assignment_replayed");
    assert(replay.assignment.commandId == assigned.assignment.commandId);
    assert(replay.assignment.jobId == assigned.assignment.jobId);

    auto changedFingerprintRequest = request();
    changedFingerprintRequest.expectedNativeTimerFingerprint =
        canonicalObservedFingerprint("45");
    const auto changedFingerprint =
        service.assign(system, changedFingerprintRequest, 122, 502);
    assert(!changedFingerprint.accepted);
    assert(changedFingerprint.reasonCode ==
           "native_timer_delete_assignment_conflict");

    auto changedTargetRequest = request();
    changedTargetRequest.backendNativeTimerId = "native_timer_45";
    const auto changedTarget =
        service.assign(system, changedTargetRequest, 122, 502);
    assert(!changedTarget.accepted);
    assert(changedTarget.reasonCode == "native_timer_delete_assignment_conflict");

    auto wrongGenerationRequest = request("op_timer_delete_generation");
    wrongGenerationRequest.backendGeneration = 8;
    const auto wrongGeneration =
        service.assign(system, wrongGenerationRequest, 123, 503);
    assert(!wrongGeneration.accepted);
    assert(wrongGeneration.reasonCode ==
           "native_timer_delete_backend_generation_conflict");

    const auto wrongActor = service.assign(
        agentActor, request("op_timer_delete_actor"), 124, 504);
    assert(!wrongActor.accepted);
    assert(wrongActor.reasonCode ==
           "invalid_native_timer_delete_assignment_request");

    auto futureClaim = request("op_timer_delete_future");
    futureClaim.controlPlaneClaimedAt = 200;
    const auto invalidTime = service.assign(system, futureClaim, 125, 505);
    assert(!invalidTime.accepted);
    assert(invalidTime.reasonCode ==
           "invalid_native_timer_delete_assignment_request");

    // Updating provider generation keeps the durable assignment but makes its
    // exact provider fence stale; the old operation cannot be reassigned.
    observeProvider(commands, providerFacts("pie_timer_delete_2", 4, 5), 130);
    const auto staleReplay = service.assign(system, request(), 131, 511);
    assert(!staleReplay.accepted);
    assert(staleReplay.reasonCode ==
           "native_timer_delete_provider_selection_stale");

    // A new operation may bind to the new current provider fence.
    const auto next = service.assign(
        system, request("op_timer_delete_2"), 132, 512);
    assert(next.accepted);
    assert(!next.replayed);
    assert(next.assignment.commandId != assigned.assignment.commandId);

    // Removing the observed capability fails closed even though ownership is
    // still configured.
    observeProvider(commands, providerFacts("pie_timer_delete_3", 5, 6, false), 140);
    const auto noObservedCapability = service.assign(
        system, request("op_timer_delete_3"), 141, 521);
    assert(!noObservedCapability.accepted);
    assert(noObservedCapability.reasonCode ==
           "local_provider_capability_not_observed");

    // The persisted Timer-delete command remains undeliverable because this
    // poll does not advertise vdr.timer.delete as an Agent command type.
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_timer_delete";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {providerFacts("pie_timer_delete_3", 5, 6, false)};
    const auto notDelivered = commands.poll(poll, "agt_timer_delete", 142);
    assert(notDelivered.accepted);
    assert(!notDelivered.assignment.present);

    // Agent-instance replacement is also a hard replay fence even if the
    // backend generation and provider identity remain otherwise valid.
    assert(database.execute(
        "UPDATE backend_agents SET agent_instance_id='inst_timer_delete_2',"
        "lease_expires_at=1000,updated_at=150 WHERE agent_id='agt_timer_delete';"));
    observeProvider(
        commands, providerFacts("pie_timer_delete_4", 6, 7), 150,
        "inst_timer_delete_2");
    const auto staleAgentReplay = service.assign(system, request(), 151, 531);
    assert(!staleAgentReplay.accepted);
    assert(staleAgentReplay.reasonCode ==
           "native_timer_delete_agent_fence_stale");

    return 0;
}

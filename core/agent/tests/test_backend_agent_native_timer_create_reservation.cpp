#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandReservation.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreatePayload.h"
#include "BackendAgentNativeTimerCreateReservation.h"
#include "Database.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
RequestSecurityContext context(ActorType type, const std::string& actor)
{
    RequestSecurityContext value;
    value.requestId = "req_timer_create_reservation_test";
    value.correlationId = "corr_timer_create_reservation_test";
    value.authenticationState = AuthenticationState::Authenticated;
    value.actor = ActorIdentity{actor, type, "test", true};
    value.device = DeviceIdentity{"dev_timer_create_reservation", true};
    value.credential = CredentialIdentity{
        "cred_timer_create_reservation", true, false, false};
    value.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return value;
}

vdrsuite::agent::BackendAgentNativeTimerCreateSpecification specification()
{
    vdrsuite::agent::BackendAgentNativeTimerCreateSpecification value;
    value.channelId = "C-1-2-3";
    value.title = "Phase 64 reserved CREATE";
    value.directory = "Tests";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1030";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

vdrsuite::agent::BackendAgentLocalProviderFacts providerFacts(
    const std::string& epoch = "pie_timer_create_1",
    std::uint64_t generation = 3,
    std::uint64_t capabilityRevision = 4,
    bool includeCreate = true)
{
    vdrsuite::agent::BackendAgentLocalProviderFacts facts;
    facts.providerId = "suitebridge:local";
    facts.providerKind = "suitebridge";
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = generation;
    facts.capabilityRevision = capabilityRevision;
    facts.available = true;
    facts.capabilities = includeCreate
        ? std::vector<std::string>{"vdr.timer.create"}
        : std::vector<std::string>{"vdr.native.probe"};
    return facts;
}

void observeProvider(
    BackendAgentCommandRepository& commands,
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts,
    std::int64_t now,
    const std::string& agentInstanceId = "inst_timer_create")
{
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = agentInstanceId;
    poll.backendGeneration = 7;
    // CREATE stays deliberately absent from executable command advertisement.
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {facts};
    const auto result = commands.poll(poll, "agt_timer_create", now);
    assert(result.accepted);
    assert(!result.assignment.present);
}

vdrsuite::agent::BackendAgentNativeTimerCreateReservationRequest request(
    const std::string& operationId = "op_timer_create_1")
{
    using namespace vdrsuite::agent;
    BackendAgentNativeTimerCreateReservationRequest value;
    value.operationId = operationId;
    value.operationRevision = "1";
    value.timerAssignmentId = "ta_timer_create_1";
    value.expectedAssignmentRevision = "2";
    value.expectedIntentRevision = "5";
    value.assignmentEpoch = 3;
    value.nativeTimerBindingId = "ntb_timer_create_1";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.expectedSpecification = specification();
    value.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(
            value.expectedSpecification);
    return value;
}
}

int main()
{
    using namespace vdrsuite::agent;

    Database database;
    assert(database.open(":memory:"));
    BackendAgentRepository agents(database);
    BackendAgentCommandRepository commands(database);
    BackendAgentCommandReservationRepository reservations(database);
    assert(agents.ensureSchema());
    assert(commands.ensureSchema());
    assert(reservations.ensureSchema());
    assert(database.execute(
        "INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,"
        "credential_id,credential_generation,agent_instance_id,"
        "backend_generation,protocol_version,software_version,"
        "heartbeat_sequence,capability_revision,last_connected_at,"
        "last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES("
        "'agt_timer_create','default','actor_timer_create','dev_timer_create',"
        "'cred_timer_create',1,'inst_timer_create',7,'vdr-suite-agent/1',"
        "'test',2,1,100,100,1000,1,1);"));

    BackendAgentNativeTimerCreateReservationService service(
        commands, reservations, agents);
    const auto system = context(ActorType::System, "system_timer_create");
    const auto agentActor = context(ActorType::Agent, "actor_timer_create");

    observeProvider(commands, providerFacts(), 101);
    const auto noOwnership = service.reserve(system, request(), 120, 500);
    assert(!noOwnership.accepted);
    assert(noOwnership.reasonCode == "local_provider_ownership_required");

    BackendAgentLocalProviderOwnership ownership;
    std::string reason;
    assert(commands.setLocalProviderOwnership(
        "default", "vdr.timer", "suitebridge:local", "suitebridge",
        {"vdr.timer.create"}, 102, ownership, reason));

    const auto reserved = service.reserve(system, request(), 120, 500);
    assert(reserved.accepted);
    assert(!reserved.replayed);
    assert(reserved.reasonCode == "native_timer_create_reserved");
    assert(reserved.assignment.commandType == "vdr.timer.create");
    assert(reserved.assignment.payloadVersion == 1);
    assert(reserved.assignment.verificationPolicy == "readback_required");
    assert(reserved.assignment.operationId == "op_timer_create_1");
    assert(reserved.assignment.backendGeneration == 7);
    assert(backendAgentCommandValidAssignment(reserved.assignment));

    // Reservation is durable but must remain invisible to Agent polling until
    // the control plane has durably transitioned the MutationOperation to dispatching.
    assert(!commands.findAssignmentForOperation(
        "default", "op_timer_create_1", "vdr.timer.create").has_value());
    const auto durable = reservations.findForOperation(
        "default", "op_timer_create_1", "vdr.timer.create");
    assert(durable.ok());
    assert(durable.reservation.assignment.commandId == reserved.assignment.commandId);
    assert(durable.reservation.localProviderSelectionPresent);
    assert(!commands.hasCapability(
        "default", "agt_timer_create", "inst_timer_create", 7,
        "vdr.timer.create"));

    BackendAgentNativeTimerCreatePayload payload;
    assert(backendAgentNativeTimerCreateParsePayload(
        reserved.assignment.payload, payload, reason));
    assert(payload.operationRevision == "1");
    assert(payload.timerAssignmentId == "ta_timer_create_1");
    assert(payload.expectedAssignmentRevision == "2");
    assert(payload.expectedIntentRevision == "5");
    assert(payload.assignmentEpoch == 3);
    assert(payload.nativeTimerBindingId == "ntb_timer_create_1");
    assert(payload.controlPlaneClaimedAt == 120);
    assert(payload.expectedSpecificationFingerprint ==
        request().expectedSpecificationFingerprint);
    assert(payload.localProviderSelection.authorityDomain == "vdr.timer");
    assert(payload.localProviderSelection.requiredCapability == "vdr.timer.create");
    assert(payload.localProviderSelection.providerInstanceEpoch ==
        "pie_timer_create_1");

    const auto replay = service.reserve(system, request(), 121, 501);
    assert(replay.accepted);
    assert(replay.replayed);
    assert(replay.reasonCode == "native_timer_create_reservation_replayed");
    assert(replay.assignment.commandId == reserved.assignment.commandId);
    assert(replay.assignment.jobId == reserved.assignment.jobId);
    assert(replay.assignment.attemptId == reserved.assignment.attemptId);
    assert(replay.assignment.payload == reserved.assignment.payload);

    auto changedBinding = request();
    changedBinding.nativeTimerBindingId = "ntb_timer_create_other";
    const auto bindingConflict = service.reserve(system, changedBinding, 122, 502);
    assert(!bindingConflict.accepted);
    assert(bindingConflict.reasonCode == "native_timer_create_reservation_conflict");

    auto changedSpecification = request();
    changedSpecification.expectedSpecification.title = "Changed title";
    changedSpecification.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(
            changedSpecification.expectedSpecification);
    const auto specificationConflict =
        service.reserve(system, changedSpecification, 122, 502);
    assert(!specificationConflict.accepted);
    assert(specificationConflict.reasonCode ==
        "native_timer_create_reservation_conflict");

    auto wrongGeneration = request("op_timer_create_generation");
    wrongGeneration.backendGeneration = 8;
    const auto fencedGeneration =
        service.reserve(system, wrongGeneration, 123, 503);
    assert(!fencedGeneration.accepted);
    assert(fencedGeneration.reasonCode ==
        "native_timer_create_backend_generation_conflict");

    const auto wrongActor = service.reserve(
        agentActor, request("op_timer_create_actor"), 124, 504);
    assert(!wrongActor.accepted);
    assert(wrongActor.reasonCode ==
        "invalid_native_timer_create_reservation_request");

    observeProvider(commands, providerFacts("pie_timer_create_2", 4, 5), 130);
    const auto staleReplay = service.reserve(system, request(), 131, 511);
    assert(!staleReplay.accepted);
    assert(staleReplay.reasonCode ==
        "native_timer_create_provider_selection_stale");

    const auto next = service.reserve(
        system, request("op_timer_create_2"), 132, 512);
    assert(next.accepted);
    assert(!next.replayed);
    assert(next.assignment.commandId != reserved.assignment.commandId);
    assert(!commands.findAssignmentForOperation(
        "default", "op_timer_create_2", "vdr.timer.create").has_value());

    observeProvider(
        commands, providerFacts("pie_timer_create_3", 5, 6, false), 140);
    const auto noCapability = service.reserve(
        system, request("op_timer_create_3"), 141, 521);
    assert(!noCapability.accepted);
    assert(noCapability.reasonCode ==
        "local_provider_capability_not_observed");

    assert(database.execute(
        "UPDATE backend_agents SET agent_instance_id='inst_timer_create_2',"
        "lease_expires_at=1000,updated_at=150 WHERE agent_id='agt_timer_create';"));
    observeProvider(
        commands, providerFacts("pie_timer_create_4", 6, 7), 150,
        "inst_timer_create_2");
    const auto staleAgent = service.reserve(system, request(), 151, 531);
    assert(!staleAgent.accepted);
    assert(staleAgent.reasonCode == "native_timer_create_agent_fence_stale");

    return 0;
}

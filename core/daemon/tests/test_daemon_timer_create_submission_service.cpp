#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandReservation.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreateActivation.h"
#include "BackendAgentNativeTimerCreateReservation.h"
#include "DaemonTimerCreateSubmissionService.h"
#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationPreparationService.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace vdrsuite::agent;
using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

TimerIntent activeIntent(TimerIntentRepository& repository)
{
    TimerIntent intent;
    intent.timerIntentId = "intent:public:create:1";
    intent.state = TimerIntentState::draft;
    intent.createdByActorId = "actor:owner";
    intent.spec.intentType = TimerIntentType::manualWindow;
    intent.spec.ownerActorId = "actor:owner";
    intent.spec.channelRequirement.canonicalChannelId = "channel:one";
    intent.spec.schedule.startAt = 1000;
    intent.spec.schedule.stopAt = 2000;
    intent.spec.schedule.timezone = "Europe/Berlin";
    intent.createdAt = 100;
    intent.updatedAt = 100;
    intent.expiresAt = 4000000000LL;

    const auto created = repository.create(intent);
    assert(created.ok());

    TimerIntent active = created.intent;
    active.state = TimerIntentState::active;
    active.updatedAt = 101;
    const auto updated =
        repository.update(active, created.intent.intentRevision);
    assert(updated.ok());
    return updated.intent;
}

TimerAssignment provisioningAssignment(
    TimerAssignmentRepository& repository,
    const TimerIntent& intent)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:public:create:1";
    assignment.timerIntentId = intent.timerIntentId;
    assignment.intentRevision = intent.intentRevision;
    assignment.backendId = "default";
    assignment.backendGeneration = 7;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding.canonicalChannelId = "channel:one";
    assignment.channelBinding.backendChannelId = "C-1-2-3";
    assignment.channelBinding.mappingSource = "canonical-channel-map";
    assignment.channelBinding.mappingRevision = "mapping:7";
    assignment.capabilityRevision = "capability:7";
    assignment.backendHealthRevision = "health:7";
    assignment.decisionPolicyVersion = "policy:1";
    assignment.decisionEvidence.reasons = {"eligible_backend"};
    assignment.createdAt = 200;
    assignment.updatedAt = 200;

    const auto created = repository.create(assignment);
    assert(created.ok());

    TimerAssignment provisioning = created.assignment;
    provisioning.state = TimerAssignmentState::provisioning;
    provisioning.updatedAt = 201;
    const auto updated =
        repository.update(
            provisioning,
            created.assignment.assignmentRevision);
    assert(updated.ok());
    return updated.assignment;
}

void seedAgent(Database& database)
{
    assert(database.execute(
        "INSERT INTO backend_agents("
        "agent_id,backend_id,actor_id,device_id,credential_id,"
        "credential_generation,agent_instance_id,backend_generation,"
        "protocol_version,software_version,heartbeat_sequence,"
        "capability_revision,last_connected_at,last_heartbeat_at,"
        "lease_expires_at,created_at,updated_at) VALUES("
        "'agt_public_create','default','actor_agent','dev_agent',"
        "'cred_agent',1,'inst_public_create',7,'vdr-suite-agent/1',"
        "'test',2,1,1,1,4000000000,1,1);"));
}

BackendAgentLocalProviderFacts providerFacts()
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = kBackendAgentNativeTimerCreateProviderId;
    facts.providerKind = kBackendAgentNativeTimerCreateProviderKind;
    facts.providerInstanceEpoch = "pie_public_create_1";
    facts.providerGeneration = 3;
    facts.capabilityRevision = 4;
    facts.available = true;
    facts.capabilities = {kBackendAgentNativeTimerCreateCapability};
    return facts;
}

void establishProvider(
    BackendAgentCommandRepository& commands)
{
    BackendAgentLocalProviderOwnership ownership;
    std::string reason;
    assert(commands.setLocalProviderOwnership(
        "default",
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateProviderId,
        kBackendAgentNativeTimerCreateProviderKind,
        {kBackendAgentNativeTimerCreateCapability},
        1,
        ownership,
        reason));

    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_public_create";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {providerFacts()};
    const auto observed =
        commands.poll(poll, "agt_public_create", 2);
    assert(observed.accepted);
    assert(!observed.assignment.present);
}

NativeTimerSpecification requestedSpecification()
{
    NativeTimerSpecification value;
    // Public submission deliberately does not own the backend channel id.
    value.channelId.clear();
    value.title = "Public CREATE";
    value.directory = "Tests";
    value.day = "2026-09-23";
    value.weekdays = "-------";
    value.startTime = "2015";
    value.endTime = "2100";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

DaemonTimerCreateSubmissionRequest requestFor(
    const TimerAssignment& assignment,
    const std::string& idempotencyKey = "idem-public-create-1")
{
    DaemonTimerCreateSubmissionRequest request;
    request.timerAssignmentId = assignment.timerAssignmentId;
    request.backendId = assignment.backendId;
    request.actorId = "actor:owner";
    request.idempotencyKey = idempotencyKey;
    request.expectedAssignmentRevision =
        assignment.assignmentRevision;
    request.requestedSpecification =
        requestedSpecification();
    request.requestId = "req-public-create";
    request.correlationId = "corr-public-create";
    return request;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    TimerIntentRepository intents(database);
    TimerAssignmentRepository assignments(database);
    MutationOperationRepository operations(database);
    BackendAgentRepository agents(database);
    BackendAgentCommandRepository commands(database);
    BackendAgentCommandReservationRepository reservations(database);

    assert(intents.ensureSchema());
    assert(assignments.ensureSchema());
    assert(operations.ensureSchema());
    assert(agents.ensureSchema());
    assert(commands.ensureSchema());
    assert(reservations.ensureSchema());

    const TimerIntent intent = activeIntent(intents);
    const TimerAssignment assignment =
        provisioningAssignment(assignments, intent);

    seedAgent(database);
    establishProvider(commands);

    NativeTimerCreateOperationPreparationService preparation(
        intents,
        assignments,
        operations);
    BackendAgentNativeTimerCreateReservationService reservation(
        commands,
        reservations,
        agents);
    NativeTimerCreateDispatchService dispatch(operations);
    BackendAgentNativeTimerCreateActivationService activation(
        operations,
        reservations,
        commands);

    DaemonTimerCreateSubmissionService service(
        intents,
        assignments,
        operations,
        preparation,
        agents,
        reservation,
        dispatch,
        activation);

    const auto request = requestFor(assignment);
    const auto first = service.submit(request);
    assert(first.status ==
        DaemonTimerCreateSubmissionStatus::accepted);
    assert(!first.operation.operationId.empty());
    assert(first.operation.actorId == "actor:owner");
    assert(first.operation.backendId == "default");
    assert(first.operation.resourceType == "TimerAssignment");
    assert(first.operation.resourceId ==
        assignment.timerAssignmentId);
    assert(first.operation.expectedRevision ==
        assignment.assignmentRevision);
    assert(first.operation.actionFamily == "timer.create");
    assert(first.operation.state ==
        MutationOperationState::dispatching);
    assert(first.operation.operationRevision == "2");

    const auto activeCommand =
        commands.findAssignmentForOperation(
            "default",
            first.operation.operationId,
            kBackendAgentNativeTimerCreateCommandType);
    assert(activeCommand.has_value());
    const std::string firstCommandId =
        activeCommand->commandId;

    const auto durablePayload =
        operations.findPayloadByOperationId(
            first.operation.operationId);
    assert(durablePayload.ok());
    assert(durablePayload.payload.payloadType ==
        "native.timer.create");

    // Exact public retry returns the same durable operation and same command.
    const auto replay = service.submit(request);
    assert(replay.status ==
        DaemonTimerCreateSubmissionStatus::accepted);
    assert(replay.operation.operationId ==
        first.operation.operationId);
    const auto replayCommand =
        commands.findAssignmentForOperation(
            "default",
            first.operation.operationId,
            kBackendAgentNativeTimerCreateCommandType);
    assert(replayCommand.has_value());
    assert(replayCommand->commandId == firstCommandId);

    // Same idempotency scope with changed desired native state fails closed.
    auto changed = request;
    changed.requestedSpecification.title = "Changed CREATE";
    const auto conflict = service.submit(changed);
    assert(conflict.status ==
        DaemonTimerCreateSubmissionStatus::idempotencyConflict);
    assert(conflict.operation.operationId ==
        first.operation.operationId);

    // A stale public If-Match translation remains an assignment revision conflict.
    auto stale = request;
    stale.expectedAssignmentRevision = "1";
    const auto staleResult = service.submit(stale);
    assert(staleResult.status ==
        DaemonTimerCreateSubmissionStatus::revisionConflict);

    // The public client cannot inject the backend-native channel identity.
    auto injectedChannel = request;
    injectedChannel.requestedSpecification.channelId =
        "C-attacker";
    const auto injected = service.submit(injectedChannel);
    assert(injected.status ==
        DaemonTimerCreateSubmissionStatus::invalid);

    return 0;
}

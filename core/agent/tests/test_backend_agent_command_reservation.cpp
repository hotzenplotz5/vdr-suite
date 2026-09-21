#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandReservation.h"
#include "Database.h"

#include <cassert>
#include <iostream>

namespace
{
BackendAgentCommandAssignment assignment(
    const std::string& commandId = "command:1",
    const std::string& operationId = "operation:1",
    std::int64_t deadline = 200)
{
    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "request:1";
    value.correlationId = "correlation:1";
    value.operationId = operationId;
    value.jobId = "job:1";
    value.attemptId = "attempt:1";
    value.claimEpoch = 1;
    value.commandId = commandId;
    value.backendId = "backend:1";
    value.agentId = "agent:1";
    value.agentInstanceId = "instance:1";
    value.backendGeneration = 7;
    value.commandType = "probe.noop";
    value.payloadVersion = 1;
    value.payload = "{}";
    value.verificationPolicy = "none";
    value.assignedAt = 100;
    value.deadline = deadline;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    BackendAgentCommandRepository commandRepository(database);
    BackendAgentCommandReservationRepository reservationRepository(database);
    assert(commandRepository.ensureSchema());
    assert(reservationRepository.ensureSchema());
    assert(database.tableExists("backend_agent_command_reservations"));

    const auto requested = assignment();
    const auto reserved = reservationRepository.reserve(requested);
    assert(reserved.status == BackendAgentCommandReservationStatus::reserved);
    assert(reserved.reservation.assignment.commandId == "command:1");
    assert(reserved.reservation.assignmentFingerprint ==
        backendAgentCommandFingerprint(requested));
    assert(!reserved.reservation.localProviderSelectionPresent);

    // Reservation is deliberately not visible to normal Agent polling/delivery.
    assert(!commandRepository.findAssignmentForOperation(
        "backend:1", "operation:1", "probe.noop").has_value());

    const auto foundById = reservationRepository.findByCommandId("command:1");
    assert(foundById.ok());
    const auto foundByOperation = reservationRepository.findForOperation(
        "backend:1", "operation:1", "probe.noop");
    assert(foundByOperation.ok());

    const auto replay = reservationRepository.reserve(requested);
    assert(replay.status == BackendAgentCommandReservationStatus::alreadyReserved);

    auto changed = assignment("command:other", "operation:1", 250);
    changed.jobId = "job:other";
    changed.attemptId = "attempt:other";
    changed.requestFingerprint = backendAgentCommandFingerprint(changed);
    assert(backendAgentCommandValidAssignment(changed));
    assert(reservationRepository.reserve(changed).status ==
        BackendAgentCommandReservationStatus::conflict);

    auto collision = assignment("command:1", "operation:other", 250);
    collision.jobId = "job:collision";
    collision.attemptId = "attempt:collision";
    collision.requestFingerprint = backendAgentCommandFingerprint(collision);
    assert(backendAgentCommandValidAssignment(collision));
    assert(reservationRepository.reserve(collision).status ==
        BackendAgentCommandReservationStatus::conflict);

    BackendAgentCommandReservationActivationService activation(
        reservationRepository, commandRepository);
    const auto activated = activation.activate("command:1");
    assert(activated.status == BackendAgentCommandReservationStatus::activated);
    assert(activated.assignment.commandId == "command:1");

    const auto active = commandRepository.findAssignmentForOperation(
        "backend:1", "operation:1", "probe.noop");
    assert(active.has_value());
    assert(active->commandId == "command:1");
    assert(active->requestFingerprint == requested.requestFingerprint);

    const auto activationReplay = activation.activate("command:1");
    assert(activationReplay.status ==
        BackendAgentCommandReservationStatus::alreadyActivated);

    // The immutable reservation remains available for crash recovery/audit.
    assert(reservationRepository.findByCommandId("command:1").ok());

    auto invalid = requested;
    invalid.commandId.clear();
    assert(reservationRepository.reserve(invalid).status ==
        BackendAgentCommandReservationStatus::invalid);
    assert(activation.activate("missing").status ==
        BackendAgentCommandReservationStatus::notFound);

    std::cout << "test_backend_agent_command_reservation passed\n";
    return 0;
}

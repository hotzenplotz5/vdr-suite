#include "BackendAgentCommandReservation.h"

#include "BackendAgentCommandDelivery.h"
#include "Database.h"

#include <sqlite3.h>

#include <cstdint>
#include <string>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindInt64(sqlite3_stmt* statement, int index, std::int64_t value)
{
    return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(value));
}

BackendAgentCommandReservationResult reservationResult(
    BackendAgentCommandReservationStatus status,
    const BackendAgentCommandReservation& reservation = {})
{
    BackendAgentCommandReservationResult value;
    value.status = status;
    value.reservation = reservation;
    return value;
}

BackendAgentCommandActivationResult activationResult(
    BackendAgentCommandReservationStatus status,
    const BackendAgentCommandAssignment& assignment = {})
{
    BackendAgentCommandActivationResult value;
    value.status = status;
    value.assignment = assignment;
    return value;
}

bool validReservationInput(
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentLocalProviderSelection* selection)
{
    if (!backendAgentCommandValidAssignment(assignment)) return false;
    if (selection == nullptr) return true;
    return vdrsuite::agent::backendAgentLocalProviderValidSelection(*selection)
        && selection->backendId == assignment.backendId
        && selection->requiredCapability == assignment.commandType
        && !vdrsuite::agent::backendAgentLocalProviderSelectionIdentity(*selection).empty();
}

BackendAgentCommandReservation makeReservation(
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentLocalProviderSelection* selection)
{
    BackendAgentCommandReservation value;
    value.assignment = assignment;
    value.assignment.present = true;
    value.assignmentFingerprint = backendAgentCommandFingerprint(assignment);
    if (selection != nullptr)
    {
        value.localProviderSelectionPresent = true;
        value.localProviderSelection = *selection;
        value.localProviderSelectionIdentity =
            vdrsuite::agent::backendAgentLocalProviderSelectionIdentity(*selection);
    }
    return value;
}

bool sameReservation(
    const BackendAgentCommandReservation& left,
    const BackendAgentCommandReservation& right)
{
    return left.assignment.commandId == right.assignment.commandId
        && left.assignmentFingerprint == right.assignmentFingerprint
        && left.localProviderSelectionPresent == right.localProviderSelectionPresent
        && left.localProviderSelectionIdentity == right.localProviderSelectionIdentity;
}

constexpr const char* kReservationColumns =
    "command_id,backend_id,operation_id,command_type,assignment_fingerprint,"
    "protocol_version,request_id,correlation_id,job_id,attempt_id,claim_epoch,"
    "agent_id,agent_instance_id,backend_generation,payload_version,payload,"
    "request_fingerprint,verification_policy,assigned_at,deadline,"
    "selection_present,selection_identity,authority_domain,provider_id,provider_kind,"
    "ownership_generation,provider_instance_epoch,provider_generation,"
    "capability_revision,required_capability";

bool readReservation(
    sqlite3_stmt* statement,
    BackendAgentCommandReservation& reservation)
{
    auto& assignment = reservation.assignment;
    assignment.present = true;
    assignment.commandId = columnText(statement, 0);
    assignment.backendId = columnText(statement, 1);
    assignment.operationId = columnText(statement, 2);
    assignment.commandType = columnText(statement, 3);
    reservation.assignmentFingerprint = columnText(statement, 4);
    assignment.protocolVersion = columnText(statement, 5);
    assignment.requestId = columnText(statement, 6);
    assignment.correlationId = columnText(statement, 7);
    assignment.jobId = columnText(statement, 8);
    assignment.attemptId = columnText(statement, 9);
    assignment.claimEpoch = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 10));
    assignment.agentId = columnText(statement, 11);
    assignment.agentInstanceId = columnText(statement, 12);
    assignment.backendGeneration = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 13));
    assignment.payloadVersion = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 14));
    assignment.payload = columnText(statement, 15);
    assignment.requestFingerprint = columnText(statement, 16);
    assignment.verificationPolicy = columnText(statement, 17);
    assignment.assignedAt = sqlite3_column_int64(statement, 18);
    assignment.deadline = sqlite3_column_int64(statement, 19);

    reservation.localProviderSelectionPresent = sqlite3_column_int(statement, 20) != 0;
    reservation.localProviderSelectionIdentity = columnText(statement, 21);
    if (reservation.localProviderSelectionPresent)
    {
        auto& selection = reservation.localProviderSelection;
        selection.backendId = assignment.backendId;
        selection.authorityDomain = columnText(statement, 22);
        selection.providerId = columnText(statement, 23);
        selection.providerKind = columnText(statement, 24);
        selection.ownershipGeneration =
            static_cast<std::uint64_t>(sqlite3_column_int64(statement, 25));
        selection.providerInstanceEpoch = columnText(statement, 26);
        selection.providerGeneration =
            static_cast<std::uint64_t>(sqlite3_column_int64(statement, 27));
        selection.capabilityRevision =
            static_cast<std::uint64_t>(sqlite3_column_int64(statement, 28));
        selection.requiredCapability = columnText(statement, 29);
    }

    if (!backendAgentCommandValidAssignment(assignment)
        || reservation.assignmentFingerprint != backendAgentCommandFingerprint(assignment))
    {
        return false;
    }
    if (!reservation.localProviderSelectionPresent)
        return reservation.localProviderSelectionIdentity.empty();

    return vdrsuite::agent::backendAgentLocalProviderValidSelection(
               reservation.localProviderSelection)
        && reservation.localProviderSelection.backendId == assignment.backendId
        && reservation.localProviderSelection.requiredCapability == assignment.commandType
        && reservation.localProviderSelectionIdentity ==
            vdrsuite::agent::backendAgentLocalProviderSelectionIdentity(
                reservation.localProviderSelection);
}

bool selectReservation(
    Database& database,
    const std::string& whereClause,
    const std::string& first,
    const std::string& second,
    const std::string& third,
    BackendAgentCommandReservation& reservation,
    bool& found)
{
    found = false;
    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + kReservationColumns
        + " FROM backend_agent_command_reservations WHERE " + whereClause
        + " LIMIT 1;";
    if (sqlite3_prepare_v2(
            database.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK)
        return false;
    bool bound = bindText(statement, 1, first);
    if (!second.empty()) bound = bound && bindText(statement, 2, second);
    if (!third.empty()) bound = bound && bindText(statement, 3, third);
    if (!bound)
    {
        sqlite3_finalize(statement);
        return false;
    }
    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        found = readReservation(statement, reservation);
        sqlite3_finalize(statement);
        return found;
    }
    sqlite3_finalize(statement);
    return step == SQLITE_DONE;
}

bool insertReservation(
    Database& database,
    const BackendAgentCommandReservation& reservation)
{
    const auto& a = reservation.assignment;
    const auto& s = reservation.localProviderSelection;
    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string("INSERT INTO backend_agent_command_reservations(")
        + kReservationColumns
        + ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    const bool prepared = sqlite3_prepare_v2(
        database.handle(), sql.c_str(), -1, &statement, nullptr) == SQLITE_OK;
    const bool bound = prepared
        && bindText(statement, 1, a.commandId)
        && bindText(statement, 2, a.backendId)
        && bindText(statement, 3, a.operationId)
        && bindText(statement, 4, a.commandType)
        && bindText(statement, 5, reservation.assignmentFingerprint)
        && bindText(statement, 6, a.protocolVersion)
        && bindText(statement, 7, a.requestId)
        && bindText(statement, 8, a.correlationId)
        && bindText(statement, 9, a.jobId)
        && bindText(statement, 10, a.attemptId)
        && bindInt64(statement, 11, static_cast<std::int64_t>(a.claimEpoch))
        && bindText(statement, 12, a.agentId)
        && bindText(statement, 13, a.agentInstanceId)
        && bindInt64(statement, 14, static_cast<std::int64_t>(a.backendGeneration))
        && bindInt64(statement, 15, static_cast<std::int64_t>(a.payloadVersion))
        && bindText(statement, 16, a.payload)
        && bindText(statement, 17, a.requestFingerprint)
        && bindText(statement, 18, a.verificationPolicy)
        && bindInt64(statement, 19, a.assignedAt)
        && bindInt64(statement, 20, a.deadline)
        && bindInt64(statement, 21, reservation.localProviderSelectionPresent ? 1 : 0)
        && bindText(statement, 22, reservation.localProviderSelectionIdentity)
        && bindText(statement, 23, reservation.localProviderSelectionPresent ? s.authorityDomain : std::string())
        && bindText(statement, 24, reservation.localProviderSelectionPresent ? s.providerId : std::string())
        && bindText(statement, 25, reservation.localProviderSelectionPresent ? s.providerKind : std::string())
        && bindInt64(statement, 26, reservation.localProviderSelectionPresent ? static_cast<std::int64_t>(s.ownershipGeneration) : 0)
        && bindText(statement, 27, reservation.localProviderSelectionPresent ? s.providerInstanceEpoch : std::string())
        && bindInt64(statement, 28, reservation.localProviderSelectionPresent ? static_cast<std::int64_t>(s.providerGeneration) : 0)
        && bindInt64(statement, 29, reservation.localProviderSelectionPresent ? static_cast<std::int64_t>(s.capabilityRevision) : 0)
        && bindText(statement, 30, reservation.localProviderSelectionPresent ? s.requiredCapability : std::string());
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    if (statement != nullptr) sqlite3_finalize(statement);
    return step == SQLITE_DONE;
}

bool activeMatchesReservation(
    BackendAgentCommandRepository& commandRepository,
    const BackendAgentCommandReservation& reservation,
    BackendAgentCommandAssignment& active)
{
    const auto found = commandRepository.findAssignmentForOperation(
        reservation.assignment.backendId,
        reservation.assignment.operationId,
        reservation.assignment.commandType);
    if (!found.has_value()) return false;
    active = *found;
    if (active.commandId != reservation.assignment.commandId
        || backendAgentCommandFingerprint(active) != reservation.assignmentFingerprint)
    {
        return false;
    }

    const auto selection =
        commandRepository.localProviderSelectionForCommand(active.commandId);
    if (!reservation.localProviderSelectionPresent)
        return !selection.has_value();
    return selection.has_value()
        && vdrsuite::agent::backendAgentLocalProviderSelectionIdentity(*selection)
            == reservation.localProviderSelectionIdentity;
}
}

BackendAgentCommandReservationRepository::BackendAgentCommandReservationRepository(
    Database& database)
    : database_(database)
{
}

bool BackendAgentCommandReservationRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_command_reservations ("
        "command_id TEXT PRIMARY KEY,backend_id TEXT NOT NULL,operation_id TEXT NOT NULL,"
        "command_type TEXT NOT NULL,assignment_fingerprint TEXT NOT NULL,"
        "protocol_version TEXT NOT NULL,request_id TEXT NOT NULL,correlation_id TEXT NOT NULL,"
        "job_id TEXT NOT NULL,attempt_id TEXT NOT NULL,claim_epoch INTEGER NOT NULL,"
        "agent_id TEXT NOT NULL,agent_instance_id TEXT NOT NULL,backend_generation INTEGER NOT NULL,"
        "payload_version INTEGER NOT NULL,payload TEXT NOT NULL,request_fingerprint TEXT NOT NULL,"
        "verification_policy TEXT NOT NULL,assigned_at INTEGER NOT NULL,deadline INTEGER NOT NULL,"
        "selection_present INTEGER NOT NULL,selection_identity TEXT NOT NULL,"
        "authority_domain TEXT NOT NULL,provider_id TEXT NOT NULL,provider_kind TEXT NOT NULL,"
        "ownership_generation INTEGER NOT NULL,provider_instance_epoch TEXT NOT NULL,"
        "provider_generation INTEGER NOT NULL,capability_revision INTEGER NOT NULL,"
        "required_capability TEXT NOT NULL,"
        "UNIQUE(backend_id,operation_id,command_type),"
        "CHECK(claim_epoch>0),CHECK(backend_generation>0),CHECK(payload_version>0),"
        "CHECK(selection_present IN (0,1))"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_backend_agent_command_reservations_operation "
        "ON backend_agent_command_reservations(backend_id,operation_id,command_type);"
    );
}

BackendAgentCommandReservationResult
BackendAgentCommandReservationRepository::reserve(
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentLocalProviderSelection* selection)
{
    if (!validReservationInput(assignment, selection))
        return reservationResult(BackendAgentCommandReservationStatus::invalid);

    const BackendAgentCommandReservation requested =
        makeReservation(assignment, selection);

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return reservationResult(BackendAgentCommandReservationStatus::storageError);
    auto rollback = [this]() { database_.execute("ROLLBACK;"); };

    BackendAgentCommandReservation existing;
    bool found = false;
    if (!selectReservation(
            database_,
            "backend_id=? AND operation_id=? AND command_type=?",
            assignment.backendId,
            assignment.operationId,
            assignment.commandType,
            existing,
            found))
    {
        rollback();
        return reservationResult(BackendAgentCommandReservationStatus::storageError);
    }
    if (found)
    {
        rollback();
        return reservationResult(
            sameReservation(existing, requested)
                ? BackendAgentCommandReservationStatus::alreadyReserved
                : BackendAgentCommandReservationStatus::conflict,
            existing);
    }

    if (!selectReservation(
            database_, "command_id=?",
            assignment.commandId, "", "",
            existing, found))
    {
        rollback();
        return reservationResult(BackendAgentCommandReservationStatus::storageError);
    }
    if (found)
    {
        rollback();
        return reservationResult(
            sameReservation(existing, requested)
                ? BackendAgentCommandReservationStatus::alreadyReserved
                : BackendAgentCommandReservationStatus::conflict,
            existing);
    }

    if (!insertReservation(database_, requested))
    {
        rollback();
        return reservationResult(BackendAgentCommandReservationStatus::storageError);
    }
    if (!database_.execute("COMMIT;"))
    {
        rollback();
        return reservationResult(BackendAgentCommandReservationStatus::storageError);
    }
    return reservationResult(
        BackendAgentCommandReservationStatus::reserved,
        requested);
}

BackendAgentCommandReservationResult
BackendAgentCommandReservationRepository::findByCommandId(
    const std::string& commandId) const
{
    if (!backendAgentCommandSafeIdentifier(commandId))
        return reservationResult(BackendAgentCommandReservationStatus::invalid);
    BackendAgentCommandReservation reservation;
    bool found = false;
    if (!selectReservation(
            database_, "command_id=?", commandId, "", "",
            reservation, found))
    {
        return reservationResult(BackendAgentCommandReservationStatus::storageError);
    }
    return found
        ? reservationResult(BackendAgentCommandReservationStatus::reserved, reservation)
        : reservationResult(BackendAgentCommandReservationStatus::notFound);
}

BackendAgentCommandReservationResult
BackendAgentCommandReservationRepository::findForOperation(
    const std::string& backendId,
    const std::string& operationId,
    const std::string& commandType) const
{
    if (!backendAgentCommandSafeIdentifier(backendId)
        || !backendAgentCommandSafeIdentifier(operationId)
        || !backendAgentCommandSafeIdentifier(commandType))
    {
        return reservationResult(BackendAgentCommandReservationStatus::invalid);
    }
    BackendAgentCommandReservation reservation;
    bool found = false;
    if (!selectReservation(
            database_,
            "backend_id=? AND operation_id=? AND command_type=?",
            backendId, operationId, commandType,
            reservation, found))
    {
        return reservationResult(BackendAgentCommandReservationStatus::storageError);
    }
    return found
        ? reservationResult(BackendAgentCommandReservationStatus::reserved, reservation)
        : reservationResult(BackendAgentCommandReservationStatus::notFound);
}

BackendAgentCommandReservationActivationService::
BackendAgentCommandReservationActivationService(
    BackendAgentCommandReservationRepository& reservationRepository,
    BackendAgentCommandRepository& commandRepository)
    : reservationRepository_(reservationRepository),
      commandRepository_(commandRepository)
{
}

BackendAgentCommandActivationResult
BackendAgentCommandReservationActivationService::activate(
    const std::string& commandId)
{
    const auto found = reservationRepository_.findByCommandId(commandId);
    if (found.status == BackendAgentCommandReservationStatus::notFound)
        return activationResult(BackendAgentCommandReservationStatus::notFound);
    if (!found.ok())
    {
        return activationResult(
            found.status == BackendAgentCommandReservationStatus::invalid
                ? BackendAgentCommandReservationStatus::invalid
                : BackendAgentCommandReservationStatus::storageError);
    }

    const BackendAgentCommandReservation& reservation = found.reservation;
    BackendAgentCommandAssignment active;
    if (activeMatchesReservation(commandRepository_, reservation, active))
    {
        return activationResult(
            BackendAgentCommandReservationStatus::alreadyActivated,
            active);
    }

    const auto existing = commandRepository_.findAssignmentForOperation(
        reservation.assignment.backendId,
        reservation.assignment.operationId,
        reservation.assignment.commandType);
    if (existing.has_value())
        return activationResult(BackendAgentCommandReservationStatus::conflict, *existing);

    const auto* selection = reservation.localProviderSelectionPresent
        ? &reservation.localProviderSelection
        : nullptr;
    if (commandRepository_.insertAssignment(reservation.assignment, selection))
    {
        return activationResult(
            BackendAgentCommandReservationStatus::activated,
            reservation.assignment);
    }

    if (activeMatchesReservation(commandRepository_, reservation, active))
    {
        return activationResult(
            BackendAgentCommandReservationStatus::alreadyActivated,
            active);
    }
    return activationResult(BackendAgentCommandReservationStatus::storageError);
}

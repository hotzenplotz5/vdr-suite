#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandReservation.h"
#include "Database.h"

#include <cassert>
#include <iostream>
#include <sqlite3.h>

namespace
{
int sqlCount(sqlite3* connection, const char* sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(connection, sql, -1, &statement, nullptr)
        == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const int count = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return count;
}

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

    // P7 additional SQL writer: persistent CREATE TABLE/INDEX from the
    // actual reservation repository, idempotent on the active connection.
    assert(reservationRepository.ensureSchema());
    assert(sqlCount(database.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='index' AND "
        "name='idx_backend_agent_command_reservations_operation';") == 1);
    assert(sqlCount(database.handle(),
        "SELECT count(*) FROM backend_agent_command_reservations "
        "WHERE command_id='command:1' AND backend_id='backend:1' "
        "AND operation_id='operation:1' "
        "AND command_type='probe.noop' AND selection_present=0;") == 1);
    assert(sqlCount(database.handle(),
        "SELECT count(*) FROM pragma_foreign_key_check;") == 0);

    // Backup and restore between two independent isolated :memory: databases.
    Database snapshot;
    assert(snapshot.open(":memory:"));
    sqlite3_backup* copy =
        sqlite3_backup_init(snapshot.handle(), "main",
                            database.handle(), "main");
    assert(copy != nullptr);
    assert(sqlite3_backup_step(copy, -1) == SQLITE_DONE);
    assert(sqlite3_backup_finish(copy) == SQLITE_OK);
    BackendAgentCommandReservationRepository reopened(snapshot);
    assert(reopened.ensureSchema());
    assert(reopened.findByCommandId("command:1").ok());
    assert(reopened.reserve(requested).status ==
        BackendAgentCommandReservationStatus::alreadyReserved);
    assert(sqlCount(snapshot.handle(),
        "SELECT count(*) FROM backend_agent_command_reservations "
        "WHERE command_id='command:1' AND operation_id='operation:1';") == 1);
    assert(sqlCount(snapshot.handle(),
        "SELECT count(*) FROM pragma_foreign_key_check;") == 0);
    // P7 additional agent assignment DDL writers. Isolated memory schema
    // deliberately has dormant capabilities; the real C++ initializers must
    // retire precisely the fenced command families without modifying probe.
    Database schemaTests;
    assert(schemaTests.open(":memory:"));
    BackendAgentCommandRepository schemaCommands(schemaTests);
    assert(schemaCommands.ensureSchema());
    assert(schemaTests.execute(
        "INSERT INTO backend_agent_command_capabilities "
        "(backend_id,agent_id,agent_instance_id,backend_generation,"
        "command_type,published_at) VALUES "
        "('backend:1','agent:1','instance:1',7,'probe.noop',1),"
        "('backend:1','agent:1','instance:1',7,'vdr.timer.update',1),"
        "('backend:1','agent:1','instance:1',7,'vdr.timer.toggle',1),"
        "('backend:1','agent:1','instance:1',7,'vdr.recording.marks.modify',1);"));

    assert(schemaCommands.ensureNativeTimerModifyAssignmentSchema());
    assert(schemaCommands.ensureNativeTimerModifyAssignmentSchema());
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM backend_agent_command_capabilities "
        "WHERE command_type IN ('vdr.timer.update','vdr.timer.toggle');") == 0);
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='index' AND "
        "name='idx_backend_agent_timer_modify_operation';") == 1);
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='trigger' AND name IN "
        "('trg_backend_agent_timer_update_dormant_capability',"
        "'trg_backend_agent_timer_toggle_dormant_capability');") == 2);

    // Cut enables delivery by retiring only its legacy dormant-capability
    // gate; record the old trigger in the private synthetic schema.
    assert(schemaTests.execute(
        "CREATE TRIGGER trg_backend_agent_recording_cut_dormant_capability "
        "BEFORE INSERT ON backend_agent_command_capabilities "
        "WHEN NEW.command_type='vdr.recording.cut' "
        "BEGIN SELECT RAISE(IGNORE); END;"));
    assert(schemaCommands.ensureRecordingCutAssignmentSchema());
    assert(schemaCommands.ensureRecordingCutAssignmentSchema());
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='trigger' AND "
        "name='trg_backend_agent_recording_cut_dormant_capability';") == 0);
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='index' AND "
        "name='idx_backend_agent_recording_cut_operation';") == 1);

    assert(schemaCommands.ensureRecordingMarksModifyAssignmentSchema());
    assert(schemaCommands.ensureRecordingMarksModifyAssignmentSchema());
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM backend_agent_command_capabilities WHERE "
        "command_type='vdr.recording.marks.modify';") == 0);
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='index' AND "
        "name='idx_backend_agent_recording_marks_modify_operation';") == 1);
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='trigger' AND "
        "name='trg_backend_agent_recording_marks_modify_dormant_capability';")
        == 1);
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM backend_agent_command_capabilities "
        "WHERE command_type='probe.noop';") == 1);
    assert(sqlCount(schemaTests.handle(),
        "SELECT count(*) FROM pragma_foreign_key_check;") == 0);

    Database ddlRestore;
    assert(ddlRestore.open(":memory:"));
    sqlite3_backup* ddlBackup =
        sqlite3_backup_init(ddlRestore.handle(), "main",
                            schemaTests.handle(), "main");
    assert(ddlBackup != nullptr);
    assert(sqlite3_backup_step(ddlBackup, -1) == SQLITE_DONE);
    assert(sqlite3_backup_finish(ddlBackup) == SQLITE_OK);
    // Do not call generic ensureSchema() here: it deliberately retires
    // operational dormant triggers. Test the restored exact schema first.
    assert(sqlCount(ddlRestore.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='index' AND name IN "
        "('idx_backend_agent_timer_modify_operation',"
        "'idx_backend_agent_recording_cut_operation',"
        "'idx_backend_agent_recording_marks_modify_operation');") == 3);
    assert(sqlCount(ddlRestore.handle(),
        "SELECT count(*) FROM sqlite_master WHERE type='trigger' AND name IN "
        "('trg_backend_agent_timer_update_dormant_capability',"
        "'trg_backend_agent_timer_toggle_dormant_capability',"
        "'trg_backend_agent_recording_marks_modify_dormant_capability');") == 3);
    assert(sqlCount(ddlRestore.handle(),
        "SELECT count(*) FROM backend_agent_command_capabilities "
        "WHERE command_type='probe.noop';") == 1);
    assert(sqlCount(ddlRestore.handle(),
        "SELECT count(*) FROM pragma_foreign_key_check;") == 0);
    std::cout << "P7_AGENT_ASSIGNMENT_DDL_WRITERS=PASS\n";

    std::cout << "P7_AGENT_RESERVATION_SCHEMA_PRESERVATION=PASS\n";
    std::cout << "test_backend_agent_command_reservation passed\n";
    return 0;
}

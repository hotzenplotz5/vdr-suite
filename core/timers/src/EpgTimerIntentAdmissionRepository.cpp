#include "EpgTimerIntentAdmissionRepository.h"
#include "Database.h"

#include <sqlite3.h>

#include <limits>
#include <string>

namespace vdrsuite::public_epg_timer
{
namespace
{
bool bind(sqlite3_stmt* statement, int index, const std::string& value)
{
    return value.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max())
        && sqlite3_bind_text(statement, index, value.data(),
            static_cast<int>(value.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string column(sqlite3_stmt* statement, int index)
{
    const unsigned char* value = sqlite3_column_text(statement, index);
    const int size = sqlite3_column_bytes(statement, index);
    return value && size >= 0
        ? std::string(reinterpret_cast<const char*>(value), size)
        : std::string();
}
}

EpgTimerIntentAdmissionRepository::EpgTimerIntentAdmissionRepository(
    Database& database) : database_(database)
{
}

bool EpgTimerIntentAdmissionRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS epg_timer_intent_admissions ("
        "actor_id TEXT NOT NULL, backend_id TEXT NOT NULL,"
        "idempotency_key TEXT NOT NULL, channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL, event_etag TEXT NOT NULL,"
        "intent_id TEXT NOT NULL, assignment_id TEXT NOT NULL,"
        "operation_id TEXT NOT NULL UNIQUE,"
        "PRIMARY KEY(actor_id,backend_id,idempotency_key),"
        "FOREIGN KEY(intent_id) REFERENCES timer_intents(timer_intent_id),"
        "FOREIGN KEY(assignment_id) REFERENCES timer_assignments(timer_assignment_id),"
        "FOREIGN KEY(operation_id) REFERENCES mutation_operations(operation_id)"
        ");");
}

AdmissionReplayResult EpgTimerIntentAdmissionRepository::lookup(
    const std::string& actorId, const Selection& selection)
{
    auto lease = database_.acquireTransactionLease();
    AdmissionReplayResult result;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT channel_id,event_id,event_etag,operation_id "
        "FROM epg_timer_intent_admissions "
        "WHERE actor_id=? AND backend_id=? AND idempotency_key=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK)
        return result;
    const bool bound = bind(statement, 1, actorId)
        && bind(statement, 2, selection.backendId)
        && bind(statement, 3, selection.idempotencyKey);
    const int rc = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    if (rc == SQLITE_DONE) result.status = AdmissionReplayStatus::absent;
    if (rc == SQLITE_ROW)
    {
        result.status = column(statement, 0) == selection.channelId
            && column(statement, 1) == selection.eventId
            && column(statement, 2) == selection.ifMatch
            ? AdmissionReplayStatus::same : AdmissionReplayStatus::conflict;
        if (result.status == AdmissionReplayStatus::same)
            result.operationId = column(statement, 3);
    }
    sqlite3_finalize(statement);
    return result;
}

bool EpgTimerIntentAdmissionRepository::record(
    const std::string& actorId, const Selection& selection,
    const std::string& intentId, const std::string& assignmentId,
    const std::string& operationId)
{
    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_timer_intent_admissions "
        "(actor_id,backend_id,idempotency_key,channel_id,event_id,"
        "event_etag,intent_id,assignment_id,operation_id) "
        "VALUES (?,?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK)
        return false;
    const bool bound = bind(statement, 1, actorId)
        && bind(statement, 2, selection.backendId)
        && bind(statement, 3, selection.idempotencyKey)
        && bind(statement, 4, selection.channelId)
        && bind(statement, 5, selection.eventId)
        && bind(statement, 6, selection.ifMatch)
        && bind(statement, 7, intentId)
        && bind(statement, 8, assignmentId)
        && bind(statement, 9, operationId);
    const int rc = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    sqlite3_finalize(statement);
    return rc == SQLITE_DONE;
}
}

#include "ContinueWatching.h"

#include "Database.h"

#include <sqlite3.h>

#include <utility>

namespace
{

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int index)
{
    const unsigned char* value = sqlite3_column_text(statement, index);
    return value == nullptr ? std::string() : reinterpret_cast<const char*>(value);
}

bool tableHasColumn(sqlite3* database, const std::string& table, const std::string& column)
{
    sqlite3_stmt* statement = nullptr;
    const std::string sql = "PRAGMA table_info(" + table + ");";
    if (sqlite3_prepare_v2(database, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK)
        return false;

    bool found = false;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        if (columnText(statement, 1) == column) {
            found = true;
            break;
        }
    }
    sqlite3_finalize(statement);
    return found;
}

bool validScope(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId)
{
    return !actorId.empty() && !backendId.empty() && !recordingId.empty();
}

} // namespace

ContinueWatchingRepository::ContinueWatchingRepository(Database& database)
    : database_(database)
{
}

bool ContinueWatchingRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS continue_watching_state ("
        "actor_id TEXT NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "recording_id TEXT NOT NULL,"
        "backend_native_id TEXT NOT NULL DEFAULT '',"
        "position_seconds INTEGER NOT NULL CHECK(position_seconds >= 0),"
        "last_activity_at TEXT NOT NULL,"
        "last_operation_id TEXT NOT NULL,"
        "PRIMARY KEY(actor_id, backend_id, recording_id)"
        ");") &&
        (tableHasColumn(database_.handle(), "continue_watching_state", "backend_native_id") ||
         database_.execute(
             "ALTER TABLE continue_watching_state "
             "ADD COLUMN backend_native_id TEXT NOT NULL DEFAULT '';")) &&
        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_continue_watching_actor_backend_activity "
            "ON continue_watching_state(actor_id, backend_id, last_activity_at DESC, recording_id);") &&
        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_continue_watching_actor_backend_native "
            "ON continue_watching_state(actor_id, backend_id, backend_native_id);");
}

bool ContinueWatchingRepository::upsert(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    const std::string& backendNativeId,
    int positionSeconds,
    const std::string& operationId)
{
    if (!validScope(actorId, backendId, recordingId) ||
        backendNativeId.empty() ||
        positionSeconds <= 0 || operationId.empty())
    {
        return false;
    }

    auto lease = database_.acquireTransactionLease();

    sqlite3_stmt* cleanup = nullptr;
    const char* cleanupSql =
        "DELETE FROM continue_watching_state "
        "WHERE actor_id=?1 AND backend_id=?2 AND backend_native_id=?3 AND recording_id<>?4;";
    if (sqlite3_prepare_v2(database_.handle(), cleanupSql, -1, &cleanup, nullptr) != SQLITE_OK)
        return false;
    const bool cleanupBound =
        bindText(cleanup, 1, actorId) &&
        bindText(cleanup, 2, backendId) &&
        bindText(cleanup, 3, backendNativeId) &&
        bindText(cleanup, 4, recordingId);
    const bool cleanupSuccess = cleanupBound && sqlite3_step(cleanup) == SQLITE_DONE;
    sqlite3_finalize(cleanup);
    if (!cleanupSuccess) return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO continue_watching_state("
        "actor_id, backend_id, recording_id, backend_native_id, position_seconds, "
        "last_activity_at, last_operation_id) "
        "VALUES(?1, ?2, ?3, ?4, ?5, strftime('%Y-%m-%dT%H:%M:%fZ','now'), ?6) "
        "ON CONFLICT(actor_id, backend_id, recording_id) DO UPDATE SET "
        "backend_native_id=excluded.backend_native_id, "
        "position_seconds=excluded.position_seconds, "
        "last_activity_at=excluded.last_activity_at, "
        "last_operation_id=excluded.last_operation_id "
        "WHERE continue_watching_state.last_operation_id <> excluded.last_operation_id;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;

    const bool bound =
        bindText(statement, 1, actorId) &&
        bindText(statement, 2, backendId) &&
        bindText(statement, 3, recordingId) &&
        bindText(statement, 4, backendNativeId) &&
        sqlite3_bind_int(statement, 5, positionSeconds) == SQLITE_OK &&
        bindText(statement, 6, operationId);
    const bool success = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success;
}

bool ContinueWatchingRepository::clear(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    const std::string& backendNativeId)
{
    if (!validScope(actorId, backendId, recordingId)) return false;

    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM continue_watching_state "
        "WHERE actor_id=?1 AND backend_id=?2 "
        "AND (recording_id=?3 OR (?4<>'' AND backend_native_id=?4));";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool bound =
        bindText(statement, 1, actorId) &&
        bindText(statement, 2, backendId) &&
        bindText(statement, 3, recordingId) &&
        bindText(statement, 4, backendNativeId);
    const bool success = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success;
}

std::vector<ContinueWatchingState>
ContinueWatchingRepository::findForActorBackend(
    const std::string& actorId,
    const std::string& backendId) const
{
    std::vector<ContinueWatchingState> states;
    if (actorId.empty() || backendId.empty()) return states;

    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT actor_id, backend_id, recording_id, backend_native_id, position_seconds, "
        "last_activity_at, last_operation_id "
        "FROM continue_watching_state "
        "WHERE actor_id=?1 AND backend_id=?2 "
        "ORDER BY last_activity_at DESC, recording_id ASC;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return states;
    if (!bindText(statement, 1, actorId) || !bindText(statement, 2, backendId)) {
        sqlite3_finalize(statement);
        return states;
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        ContinueWatchingState state;
        state.actorId = columnText(statement, 0);
        state.backendId = columnText(statement, 1);
        state.recordingId = columnText(statement, 2);
        state.backendNativeId = columnText(statement, 3);
        state.positionSeconds = sqlite3_column_int(statement, 4);
        state.lastActivityAt = columnText(statement, 5);
        state.lastOperationId = columnText(statement, 6);
        states.push_back(std::move(state));
    }
    sqlite3_finalize(statement);
    return states;
}

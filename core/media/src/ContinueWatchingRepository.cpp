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
        "revision INTEGER NOT NULL DEFAULT 1 CHECK(revision >= 1),"
        "PRIMARY KEY(actor_id, backend_id, recording_id)"
        ");") &&
        (tableHasColumn(database_.handle(), "continue_watching_state", "backend_native_id") ||
         database_.execute(
             "ALTER TABLE continue_watching_state "
             "ADD COLUMN backend_native_id TEXT NOT NULL DEFAULT '';")) &&
        (tableHasColumn(database_.handle(), "continue_watching_state", "revision") ||
         database_.execute(
             "ALTER TABLE continue_watching_state "
             "ADD COLUMN revision INTEGER NOT NULL DEFAULT 1;")) &&
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
        "last_operation_id=excluded.last_operation_id, "
        "revision=continue_watching_state.revision+1 "
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


ContinueWatchingMutationResult ContinueWatchingRepository::updateConditional(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    const std::string& backendNativeId,
    int positionSeconds,
    const std::string& operationId,
    std::int64_t expectedRevision)
{
    using Status = ContinueWatchingMutationStatus;
    if (!validScope(actorId, backendId, recordingId) ||
        backendNativeId.empty() || operationId.empty() ||
        operationId.size() > 128U || positionSeconds < 0 ||
        expectedRevision < 0)
        return {Status::invalid, 0};

    // The same Database lease also serializes the read/compare/write.
    // No client-supplied actor, backend or native path is used here.
    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* lookup = nullptr;
    const char* selectSql =
        "SELECT recording_id, backend_native_id, position_seconds, last_operation_id, revision "
        "FROM continue_watching_state "
        "WHERE actor_id=?1 AND backend_id=?2 "
        "AND (recording_id=?3 OR backend_native_id=?4) "
        "ORDER BY CASE WHEN recording_id=?3 THEN 0 ELSE 1 END LIMIT 1;";
    if (sqlite3_prepare_v2(database_.handle(), selectSql, -1, &lookup, nullptr) != SQLITE_OK)
        return {Status::unavailable, 0};
    const bool bound = bindText(lookup, 1, actorId) &&
        bindText(lookup, 2, backendId) && bindText(lookup, 3, recordingId) &&
        bindText(lookup, 4, backendNativeId);
    if (!bound) {
        sqlite3_finalize(lookup);
        return {Status::unavailable, 0};
    }
    const int step = sqlite3_step(lookup);
    if (step != SQLITE_ROW && step != SQLITE_DONE) {
        sqlite3_finalize(lookup);
        return {Status::unavailable, 0};
    }
    const bool exists = step == SQLITE_ROW;
    const std::string existingRecording = exists ? columnText(lookup, 0) : "";
    const std::string existingNative = exists ? columnText(lookup, 1) : "";
    const int oldPosition = exists ? sqlite3_column_int(lookup, 2) : 0;
    const std::string oldOperation = exists ? columnText(lookup, 3) : "";
    const std::int64_t revision = exists ? sqlite3_column_int64(lookup, 4) : 0;
    sqlite3_finalize(lookup);

    // A changed native identity is not evidence of the same Recording.
    // Public rec_ lifecycle rebind must be independently verified first.
    if (exists && existingNative != backendNativeId)
        return {Status::conflict, revision};
    if (exists && oldOperation == operationId)
        return {oldPosition == positionSeconds ? Status::replayed : Status::conflict, revision};
    if (revision != expectedRevision || revision == INT64_MAX)
        return {Status::conflict, revision};

    sqlite3_stmt* write = nullptr;
    const char* insertSql =
        "INSERT INTO continue_watching_state("
        "actor_id,backend_id,recording_id,backend_native_id,position_seconds,"
        "last_activity_at,last_operation_id,revision) "
        "VALUES(?1,?2,?3,?4,?5,strftime('%Y-%m-%dT%H:%M:%fZ','now'),?6,1);";
    const char* updateSql =
        "UPDATE continue_watching_state SET recording_id=?3,position_seconds=?5,"
        "last_activity_at=strftime('%Y-%m-%dT%H:%M:%fZ','now'),"
        "last_operation_id=?6,revision=revision+1 "
        "WHERE actor_id=?1 AND backend_id=?2 AND recording_id=?8 "
        "AND backend_native_id=?4 AND revision=?7;";
    if (sqlite3_prepare_v2(database_.handle(), exists ? updateSql : insertSql,
                           -1, &write, nullptr) != SQLITE_OK)
        return {Status::unavailable, revision};
    bool writeBound =
        bindText(write, 1, actorId) && bindText(write, 2, backendId) &&
        bindText(write, 3, recordingId) && bindText(write, 4, backendNativeId) &&
        sqlite3_bind_int(write, 5, positionSeconds) == SQLITE_OK &&
        bindText(write, 6, operationId);
    if (exists) writeBound = writeBound &&
        sqlite3_bind_int64(write, 7, expectedRevision) == SQLITE_OK &&
        bindText(write, 8, existingRecording);
    const bool ok = writeBound && sqlite3_step(write) == SQLITE_DONE &&
        sqlite3_changes(database_.handle()) == 1;
    sqlite3_finalize(write);
    if (!ok) return {Status::unavailable, revision};
    return {Status::applied, revision + 1};
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
        "last_activity_at, last_operation_id, revision "
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
        state.revision = sqlite3_column_int64(statement, 7);
        states.push_back(std::move(state));
    }
    sqlite3_finalize(statement);
    return states;
}

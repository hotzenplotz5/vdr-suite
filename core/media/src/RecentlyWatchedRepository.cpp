#include "RecentlyWatched.h"

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

bool validScope(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId)
{
    return !actorId.empty() && !backendId.empty() && !recordingId.empty();
}

} // namespace

RecentlyWatchedRepository::RecentlyWatchedRepository(Database& database)
    : database_(database)
{
}

bool RecentlyWatchedRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS recently_watched_state ("
        "actor_id TEXT NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "recording_id TEXT NOT NULL,"
        "position_seconds INTEGER NOT NULL CHECK(position_seconds >= 0),"
        "position_known INTEGER NOT NULL CHECK(position_known IN (0,1)),"
        "completion_known INTEGER NOT NULL CHECK(completion_known IN (0,1)),"
        "completed INTEGER NOT NULL CHECK(completed IN (0,1)),"
        "resume_relevance_known INTEGER NOT NULL CHECK(resume_relevance_known IN (0,1)),"
        "resume_relevant INTEGER NOT NULL CHECK(resume_relevant IN (0,1)),"
        "source_evidence TEXT NOT NULL,"
        "last_activity_at TEXT NOT NULL,"
        "last_operation_id TEXT NOT NULL,"
        "PRIMARY KEY(actor_id, backend_id, recording_id)"
        ");") &&
        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_recently_watched_actor_backend_activity "
            "ON recently_watched_state(actor_id, backend_id, last_activity_at DESC, recording_id);");
}

bool RecentlyWatchedRepository::record(const RecentlyWatchedState& state)
{
    if (!validScope(state.actorId, state.backendId, state.recordingId) ||
        state.positionSeconds < 0 || state.sourceEvidence.empty() ||
        state.lastOperationId.empty())
    {
        return false;
    }

    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO recently_watched_state("
        "actor_id, backend_id, recording_id, position_seconds, position_known, "
        "completion_known, completed, resume_relevance_known, resume_relevant, "
        "source_evidence, last_activity_at, last_operation_id) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, "
        "strftime('%Y-%m-%dT%H:%M:%fZ','now'), ?11) "
        "ON CONFLICT(actor_id, backend_id, recording_id) DO UPDATE SET "
        "position_seconds=excluded.position_seconds, "
        "position_known=excluded.position_known, "
        "completion_known=excluded.completion_known, "
        "completed=excluded.completed, "
        "resume_relevance_known=excluded.resume_relevance_known, "
        "resume_relevant=excluded.resume_relevant, "
        "source_evidence=excluded.source_evidence, "
        "last_activity_at=excluded.last_activity_at, "
        "last_operation_id=excluded.last_operation_id "
        "WHERE recently_watched_state.last_operation_id <> excluded.last_operation_id;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;

    const bool bound =
        bindText(statement, 1, state.actorId) &&
        bindText(statement, 2, state.backendId) &&
        bindText(statement, 3, state.recordingId) &&
        sqlite3_bind_int(statement, 4, state.positionSeconds) == SQLITE_OK &&
        sqlite3_bind_int(statement, 5, state.positionKnown ? 1 : 0) == SQLITE_OK &&
        sqlite3_bind_int(statement, 6, state.completionKnown ? 1 : 0) == SQLITE_OK &&
        sqlite3_bind_int(statement, 7, state.completed ? 1 : 0) == SQLITE_OK &&
        sqlite3_bind_int(statement, 8, state.resumeRelevanceKnown ? 1 : 0) == SQLITE_OK &&
        sqlite3_bind_int(statement, 9, state.resumeRelevant ? 1 : 0) == SQLITE_OK &&
        bindText(statement, 10, state.sourceEvidence) &&
        bindText(statement, 11, state.lastOperationId);
    const bool success = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    if (!success) return false;

    return prune(state.actorId, state.backendId);
}

bool RecentlyWatchedRepository::prune(
    const std::string& actorId,
    const std::string& backendId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM recently_watched_state "
        "WHERE actor_id=?1 AND backend_id=?2 AND rowid NOT IN ("
        "SELECT rowid FROM recently_watched_state "
        "WHERE actor_id=?1 AND backend_id=?2 "
        "ORDER BY last_activity_at DESC, recording_id ASC LIMIT 100"
        ");";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool bound = bindText(statement, 1, actorId) && bindText(statement, 2, backendId);
    const bool success = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success;
}

bool RecentlyWatchedRepository::remove(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId)
{
    if (!validScope(actorId, backendId, recordingId)) return false;

    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM recently_watched_state "
        "WHERE actor_id=?1 AND backend_id=?2 AND recording_id=?3;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool bound =
        bindText(statement, 1, actorId) &&
        bindText(statement, 2, backendId) &&
        bindText(statement, 3, recordingId);
    const bool success = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success;
}

std::vector<RecentlyWatchedState> RecentlyWatchedRepository::findForActorBackend(
    const std::string& actorId,
    const std::string& backendId) const
{
    std::vector<RecentlyWatchedState> states;
    if (actorId.empty() || backendId.empty()) return states;

    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT actor_id, backend_id, recording_id, position_seconds, position_known, "
        "completion_known, completed, resume_relevance_known, resume_relevant, "
        "source_evidence, last_activity_at, last_operation_id "
        "FROM recently_watched_state "
        "WHERE actor_id=?1 AND backend_id=?2 "
        "ORDER BY last_activity_at DESC, recording_id ASC LIMIT 100;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return states;
    if (!bindText(statement, 1, actorId) || !bindText(statement, 2, backendId)) {
        sqlite3_finalize(statement);
        return states;
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        RecentlyWatchedState state;
        state.actorId = columnText(statement, 0);
        state.backendId = columnText(statement, 1);
        state.recordingId = columnText(statement, 2);
        state.positionSeconds = sqlite3_column_int(statement, 3);
        state.positionKnown = sqlite3_column_int(statement, 4) != 0;
        state.completionKnown = sqlite3_column_int(statement, 5) != 0;
        state.completed = sqlite3_column_int(statement, 6) != 0;
        state.resumeRelevanceKnown = sqlite3_column_int(statement, 7) != 0;
        state.resumeRelevant = sqlite3_column_int(statement, 8) != 0;
        state.sourceEvidence = columnText(statement, 9);
        state.lastActivityAt = columnText(statement, 10);
        state.lastOperationId = columnText(statement, 11);
        states.push_back(std::move(state));
    }
    sqlite3_finalize(statement);
    return states;
}

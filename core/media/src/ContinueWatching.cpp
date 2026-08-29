#include "ContinueWatching.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
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
        "position_seconds INTEGER NOT NULL CHECK(position_seconds >= 0),"
        "last_activity_at TEXT NOT NULL,"
        "last_operation_id TEXT NOT NULL,"
        "PRIMARY KEY(actor_id, backend_id, recording_id)"
        ");") &&
        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_continue_watching_actor_backend_activity "
            "ON continue_watching_state(actor_id, backend_id, last_activity_at DESC, recording_id);");
}

bool ContinueWatchingRepository::upsert(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    int positionSeconds,
    const std::string& operationId)
{
    if (!validScope(actorId, backendId, recordingId) ||
        positionSeconds <= 0 || operationId.empty())
    {
        return false;
    }

    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO continue_watching_state("
        "actor_id, backend_id, recording_id, position_seconds, last_activity_at, last_operation_id) "
        "VALUES(?1, ?2, ?3, ?4, strftime('%Y-%m-%dT%H:%M:%fZ','now'), ?5) "
        "ON CONFLICT(actor_id, backend_id, recording_id) DO UPDATE SET "
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
        sqlite3_bind_int(statement, 4, positionSeconds) == SQLITE_OK &&
        bindText(statement, 5, operationId);
    const bool success = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success;
}

bool ContinueWatchingRepository::clear(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId)
{
    if (!validScope(actorId, backendId, recordingId)) return false;

    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM continue_watching_state "
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
        "SELECT actor_id, backend_id, recording_id, position_seconds, "
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
        state.positionSeconds = sqlite3_column_int(statement, 3);
        state.lastActivityAt = columnText(statement, 4);
        state.lastOperationId = columnText(statement, 5);
        states.push_back(std::move(state));
    }
    sqlite3_finalize(statement);
    return states;
}

ContinueWatchingService::ContinueWatchingService(
    ContinueWatchingRepository& repository,
    RecordingResolver resolver)
    : repository_(repository),
      resolver_(std::move(resolver))
{
}

bool ContinueWatchingService::recordProgress(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    int positionSeconds,
    bool resumeSupported,
    const std::string& operationId)
{
    if (!validScope(actorId, backendId, recordingId) || operationId.empty())
        return false;

    const auto current = resolver_ ? resolver_(backendId, recordingId) : std::nullopt;
    if (!current.has_value() ||
        !current->playbackCapable ||
        !resumeSupported ||
        positionSeconds <= 0 ||
        (current->durationKnown && current->durationSeconds > 0 &&
         positionSeconds >= current->durationSeconds))
    {
        return repository_.clear(actorId, backendId, recordingId);
    }

    return repository_.upsert(
        actorId,
        backendId,
        recordingId,
        positionSeconds,
        operationId);
}

bool ContinueWatchingService::clear(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    const std::string&)
{
    return repository_.clear(actorId, backendId, recordingId);
}

std::vector<ContinueWatchingItem> ContinueWatchingService::list(
    const std::string& actorId,
    const std::string& backendId)
{
    std::vector<ContinueWatchingItem> items;
    const auto states = repository_.findForActorBackend(actorId, backendId);
    items.reserve(states.size());

    for (const auto& state : states) {
        const auto current = resolver_ ? resolver_(state.backendId, state.recordingId) : std::nullopt;
        const bool invalid =
            !current.has_value() ||
            !current->playbackCapable ||
            state.positionSeconds <= 0 ||
            (current->durationKnown && current->durationSeconds > 0 &&
             state.positionSeconds >= current->durationSeconds);
        if (invalid) {
            repository_.clear(state.actorId, state.backendId, state.recordingId);
            continue;
        }
        ContinueWatchingItem item;
        item.recording = *current;
        item.resumePositionSeconds = state.positionSeconds;
        item.lastActivityAt = state.lastActivityAt;
        items.push_back(std::move(item));
    }
    return items;
}

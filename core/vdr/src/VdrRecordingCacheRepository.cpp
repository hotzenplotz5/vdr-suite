#include "VdrRecordingCacheRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <string>
#include <vector>

namespace
{

std::string columnText(
    sqlite3_stmt* stmt,
    int column)
{
    const unsigned char* text =
        sqlite3_column_text(stmt, column);

    if (!text)
    {
        return {};
    }

    return reinterpret_cast<const char*>(text);
}

void bindText(
    sqlite3_stmt* stmt,
    int index,
    const std::string& value)
{
    sqlite3_bind_text(
        stmt,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT);
}

VdrRecording readRecording(
    sqlite3_stmt* stmt)
{
    VdrRecording recording;

    recording.id = columnText(stmt, 0);
    recording.backendId = columnText(stmt, 1);
    recording.backendNativeId = columnText(stmt, 2);
    recording.title = columnText(stmt, 3);
    recording.path = columnText(stmt, 4);
    recording.startTime = columnText(stmt, 5);
    recording.durationSeconds = sqlite3_column_int(stmt, 6);
    recording.sizeMb = sqlite3_column_int64(stmt, 7);

    return recording;
}

}

VdrRecordingCacheRepository::VdrRecordingCacheRepository(
    Database& database)
    : database_(database)
{
}

bool VdrRecordingCacheRepository::ensureSchema()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return database_.execute(
        "CREATE TABLE IF NOT EXISTS vdr_recording_cache ("
        "backend_id TEXT NOT NULL,"
        "cache_key TEXT NOT NULL,"
        "recording_id TEXT NOT NULL DEFAULT '',"
        "backend_native_id TEXT NOT NULL DEFAULT '',"
        "title TEXT NOT NULL DEFAULT '',"
        "path TEXT NOT NULL DEFAULT '',"
        "start_time TEXT NOT NULL DEFAULT '',"
        "duration_seconds INTEGER NOT NULL DEFAULT 0,"
        "size_mb INTEGER NOT NULL DEFAULT 0,"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "last_seen_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, cache_key)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_vdr_recording_cache_backend_title "
        "ON vdr_recording_cache (backend_id, title);"
        "CREATE INDEX IF NOT EXISTS idx_vdr_recording_cache_backend_start "
        "ON vdr_recording_cache (backend_id, start_time);"
        "CREATE INDEX IF NOT EXISTS idx_vdr_recording_cache_backend_path "
        "ON vdr_recording_cache (backend_id, path);");
}

bool VdrRecordingCacheRepository::upsertRecordingsForBackend(
    const std::string& backendId,
    const std::vector<VdrRecording>& recordings)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!ensureSchema())
    {
        return false;
    }

    if (recordings.empty())
    {
        return true;
    }

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return false;
    }

    const bool ok =
        upsertRecordingsForBackendLocked(
            normalizeBackendId(backendId),
            recordings);

    if (!ok)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    return true;
}

bool VdrRecordingCacheRepository::replaceRecordingsForBackend(
    const std::string& backendId,
    const std::vector<VdrRecording>& recordings)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!ensureSchema())
    {
        return false;
    }

    const std::string normalizedBackendId =
        normalizeBackendId(backendId);

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return false;
    }

    sqlite3_stmt* deleteStmt = nullptr;

    const char* deleteSql =
        "DELETE FROM vdr_recording_cache "
        "WHERE backend_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            deleteSql,
            -1,
            &deleteStmt,
            nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    bindText(deleteStmt, 1, normalizedBackendId);

    const bool deleteOk =
        sqlite3_step(deleteStmt) == SQLITE_DONE;

    sqlite3_finalize(deleteStmt);

    if (!deleteOk)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    if (!upsertRecordingsForBackendLocked(
            normalizedBackendId,
            recordings))
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    return true;
}

bool VdrRecordingCacheRepository::upsertRecordingsForBackendLocked(
    const std::string& normalizedBackendId,
    const std::vector<VdrRecording>& recordings)
{
    if (recordings.empty())
    {
        return true;
    }

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "INSERT INTO vdr_recording_cache ("
        "backend_id, cache_key, recording_id, backend_native_id, "
        "title, path, start_time, duration_seconds, size_mb, "
        "updated_at, last_seen_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id, cache_key) DO UPDATE SET "
        "recording_id = excluded.recording_id, "
        "backend_native_id = excluded.backend_native_id, "
        "title = excluded.title, "
        "path = excluded.path, "
        "start_time = excluded.start_time, "
        "duration_seconds = excluded.duration_seconds, "
        "size_mb = excluded.size_mb, "
        "updated_at = CURRENT_TIMESTAMP, "
        "last_seen_at = CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    for (const VdrRecording& recording : recordings)
    {
        const std::string cacheKey =
            cacheKeyForRecording(recording);

        if (cacheKey.empty())
        {
            continue;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        bindText(stmt, 1, normalizedBackendId);
        bindText(stmt, 2, cacheKey);
        bindText(stmt, 3, recording.id);
        bindText(stmt, 4, recording.backendNativeId);
        bindText(stmt, 5, recording.title);
        bindText(stmt, 6, recording.path);
        bindText(stmt, 7, recording.startTime);
        sqlite3_bind_int(stmt, 8, recording.durationSeconds);
        sqlite3_bind_int64(stmt, 9, recording.sizeMb);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            return false;
        }
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        return false;
    }

    return true;
}

std::vector<VdrRecording> VdrRecordingCacheRepository::findAllForBackend(
    const std::string& backendId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT recording_id, backend_id, backend_native_id, "
        "title, path, start_time, duration_seconds, size_mb "
        "FROM vdr_recording_cache "
        "WHERE backend_id = ? "
        "ORDER BY title COLLATE NOCASE ASC, path COLLATE NOCASE ASC;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return {};
    }

    bindText(stmt, 1, normalizeBackendId(backendId));

    std::vector<VdrRecording> recordings;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        recordings.push_back(readRecording(stmt));
    }

    sqlite3_finalize(stmt);

    return recordings;
}

int VdrRecordingCacheRepository::countForBackend(
    const std::string& backendId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT COUNT(*) FROM vdr_recording_cache "
        "WHERE backend_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return 0;
    }

    bindText(stmt, 1, normalizeBackendId(backendId));

    int count = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return count;
}

std::string VdrRecordingCacheRepository::normalizeBackendId(
    const std::string& backendId)
{
    if (backendId.empty())
    {
        return "default";
    }

    return backendId;
}

std::string VdrRecordingCacheRepository::cacheKeyForRecording(
    const VdrRecording& recording)
{
    if (!recording.backendNativeId.empty())
    {
        return recording.backendNativeId;
    }

    if (!recording.path.empty())
    {
        return recording.path;
    }

    if (!recording.id.empty())
    {
        return recording.id;
    }

    return recording.title;
}

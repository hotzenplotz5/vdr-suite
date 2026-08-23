#include "VdrRecordingCacheRepository.h"

#include "Database.h"
#include "VdrRecordingMetadataCacheCodec.h"

#include <sqlite3.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <utility>
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

bool tableHasColumn(
    sqlite3* database,
    const std::string& tableName,
    const std::string& columnName)
{
    sqlite3_stmt* stmt = nullptr;
    const std::string sql =
        "PRAGMA table_info(" + tableName + ");";

    if (sqlite3_prepare_v2(
            database,
            sql.c_str(),
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (columnText(stmt, 1) == columnName)
        {
            found = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

bool endsWith(
    const std::string& value,
    const std::string& suffix)
{
    if (suffix.size() > value.size())
    {
        return false;
    }

    return value.compare(
        value.size() - suffix.size(),
        suffix.size(),
        suffix) == 0;
}

bool startsWith(
    const std::string& value,
    const std::string& prefix)
{
    if (prefix.size() > value.size())
    {
        return false;
    }

    return value.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> splitPath(
    std::string path)
{
    const std::string srvPrefix =
        "/srv/vdr/video/";

    if (startsWith(path, srvPrefix))
    {
        path = path.substr(srvPrefix.size());
    }

    std::replace(path.begin(), path.end(), '\\', '/');

    while (!path.empty() && path.front() == '/')
    {
        path.erase(path.begin());
    }

    while (!path.empty() && path.back() == '/')
    {
        path.pop_back();
    }

    std::vector<std::string> result;
    std::stringstream stream(path);
    std::string segment;

    while (std::getline(stream, segment, '/'))
    {
        if (!segment.empty())
        {
            result.push_back(segment);
        }
    }

    return result;
}

std::string joinPath(
    const std::vector<std::string>& segments)
{
    std::string result;

    for (const auto& segment : segments)
    {
        if (!result.empty())
        {
            result += "/";
        }

        result += segment;
    }

    return result;
}

bool isStorageMountSegment(
    const std::string& segment)
{
    return segment == "Recordings_on_yavdr(nfs)";
}

void removeStorageMountSegments(
    std::vector<std::string>& segments)
{
    segments.erase(
        std::remove_if(
            segments.begin(),
            segments.end(),
            isStorageMountSegment),
        segments.end());
}

std::string recordingDeduplicationKey(
    const VdrRecording& recording)
{
    const std::string path =
        recording.path.empty()
            ? recording.backendNativeId
            : recording.path;

    std::vector<std::string> segments =
        splitPath(path);

    removeStorageMountSegments(segments);

    if (!segments.empty())
    {
        return joinPath(segments);
    }

    std::vector<std::string> titleSegments =
        splitPath(recording.title);

    removeStorageMountSegments(titleSegments);

    if (!titleSegments.empty())
    {
        return joinPath(titleSegments);
    }

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

std::vector<VdrRecording> uniqueRecordingsByNormalizedPath(
    const std::vector<VdrRecording>& recordings)
{
    std::map<std::string, VdrRecording> uniqueRecordings;

    for (const VdrRecording& recording : recordings)
    {
        const std::string key =
            recordingDeduplicationKey(recording);

        if (uniqueRecordings.find(key) == uniqueRecordings.end())
        {
            uniqueRecordings.emplace(key, recording);
        }
    }

    std::vector<VdrRecording> result;
    result.reserve(uniqueRecordings.size());

    for (const auto& entry : uniqueRecordings)
    {
        result.push_back(entry.second);
    }

    return result;
}

std::string lastSegment(
    const std::string& path)
{
    const std::size_t separator =
        path.rfind('/');

    if (separator == std::string::npos)
    {
        return path;
    }

    return path.substr(separator + 1);
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
    recording.metadata =
        VdrRecordingMetadataCacheCodec::decode(
            columnText(stmt, 8));
    recording.recordingDurationKnown =
        sqlite3_column_int(stmt, 9) != 0;

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

    if (!database_.execute(
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
            "metadata_payload TEXT NOT NULL DEFAULT '',"
            "recording_duration_known INTEGER NOT NULL DEFAULT 0,"
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
            "ON vdr_recording_cache (backend_id, path);"
            "CREATE TABLE IF NOT EXISTS vdr_recording_cache_status ("
            "backend_id TEXT PRIMARY KEY,"
            "state TEXT NOT NULL DEFAULT 'empty',"
            "total_count INTEGER NOT NULL DEFAULT 0,"
            "started_at TEXT NOT NULL DEFAULT '',"
            "finished_at TEXT NOT NULL DEFAULT '',"
            "last_error TEXT NOT NULL DEFAULT '',"
            "updated_at TEXT DEFAULT CURRENT_TIMESTAMP"
            ");"))
    {
        return false;
    }

    if (!tableHasColumn(
            database_.handle(),
            "vdr_recording_cache",
            "metadata_payload"))
    {
        if (!database_.execute(
                "ALTER TABLE vdr_recording_cache "
                "ADD COLUMN metadata_payload TEXT NOT NULL DEFAULT '';"))
        {
            return false;
        }
    }

    if (!tableHasColumn(
            database_.handle(),
            "vdr_recording_cache",
            "recording_duration_known"))
    {
        if (!database_.execute(
                "ALTER TABLE vdr_recording_cache "
                "ADD COLUMN recording_duration_known INTEGER NOT NULL DEFAULT 0;"))
        {
            return false;
        }
    }

    return true;
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

    const std::string normalizedBackendId =
        normalizeBackendId(backendId);

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return false;
    }

    const bool ok =
        upsertRecordingsForBackendLocked(
            normalizedBackendId,
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

    return rebuildBrowseSnapshotFromPersistentCacheLocked(
        normalizedBackendId);
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

    auto transactionLease = database_.acquireTransactionLease();
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

    storeBrowseSnapshotLocked(
        normalizedBackendId,
        recordings);

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
        "title, path, start_time, duration_seconds, size_mb, metadata_payload, "
        "recording_duration_known, updated_at, last_seen_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id, cache_key) DO UPDATE SET "
        "recording_id = excluded.recording_id, "
        "backend_native_id = excluded.backend_native_id, "
        "title = excluded.title, "
        "path = excluded.path, "
        "start_time = excluded.start_time, "
        "duration_seconds = excluded.duration_seconds, "
        "size_mb = excluded.size_mb, "
        "metadata_payload = excluded.metadata_payload, "
        "recording_duration_known = excluded.recording_duration_known, "
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
        bindText(
            stmt,
            10,
            VdrRecordingMetadataCacheCodec::encode(
                recording.metadata));
        sqlite3_bind_int(
            stmt,
            11,
            recording.recordingDurationKnown ? 1 : 0);

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
        "title, path, start_time, duration_seconds, size_mb, metadata_payload, "
        "recording_duration_known "
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

bool VdrRecordingCacheRepository::markRefreshStarted(
    const std::string& backendId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!ensureSchema())
    {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "INSERT INTO vdr_recording_cache_status ("
        "backend_id, state, total_count, started_at, finished_at, last_error, updated_at) "
        "VALUES (?, 'warming', ?, CURRENT_TIMESTAMP, '', '', CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "state = 'warming', "
        "started_at = CURRENT_TIMESTAMP, "
        "last_error = '', "
        "updated_at = CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bindText(stmt, 1, normalizeBackendId(backendId));
    sqlite3_bind_int(stmt, 2, countForBackend(backendId));

    const bool ok =
        sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    return ok;
}

bool VdrRecordingCacheRepository::markRefreshFinished(
    const std::string& backendId,
    int totalCount)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!ensureSchema())
    {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "INSERT INTO vdr_recording_cache_status ("
        "backend_id, state, total_count, started_at, finished_at, last_error, updated_at) "
        "VALUES (?, 'ready', ?, '', CURRENT_TIMESTAMP, '', CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "state = 'ready', "
        "total_count = excluded.total_count, "
        "finished_at = CURRENT_TIMESTAMP, "
        "last_error = '', "
        "updated_at = CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bindText(stmt, 1, normalizeBackendId(backendId));
    sqlite3_bind_int(stmt, 2, std::max(0, totalCount));

    const bool ok =
        sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    return ok;
}

bool VdrRecordingCacheRepository::markRefreshFailed(
    const std::string& backendId,
    const std::string& errorMessage)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!ensureSchema())
    {
        return false;
    }

    const int existingCount =
        countForBackend(backendId);

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "INSERT INTO vdr_recording_cache_status ("
        "backend_id, state, total_count, started_at, finished_at, last_error, updated_at) "
        "VALUES (?, ?, ?, '', '', ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "state = excluded.state, "
        "total_count = excluded.total_count, "
        "last_error = excluded.last_error, "
        "updated_at = CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bindText(stmt, 1, normalizeBackendId(backendId));
    bindText(stmt, 2, existingCount > 0 ? "stale" : "error");
    sqlite3_bind_int(stmt, 3, existingCount);
    bindText(stmt, 4, errorMessage);

    const bool ok =
        sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    return ok;
}

VdrRecordingCacheStatus VdrRecordingCacheRepository::statusForBackend(
    const std::string& backendId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    VdrRecordingCacheStatus status;
    status.backendId = normalizeBackendId(backendId);
    status.totalCount = countForBackend(status.backendId);

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT state, total_count, started_at, finished_at, last_error "
        "FROM vdr_recording_cache_status "
        "WHERE backend_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        status.state = status.totalCount > 0 ? "ready" : "empty";
        return status;
    }

    bindText(stmt, 1, status.backendId);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        status.state = columnText(stmt, 0);
        status.totalCount = sqlite3_column_int(stmt, 1);
        status.startedAt = columnText(stmt, 2);
        status.finishedAt = columnText(stmt, 3);
        status.lastError = columnText(stmt, 4);
    }
    else
    {
        status.state = status.totalCount > 0 ? "ready" : "empty";
    }

    sqlite3_finalize(stmt);

    if (status.totalCount <= 0)
    {
        status.totalCount = countForBackend(status.backendId);
    }

    return status;
}

bool VdrRecordingCacheRepository::warmBrowseSnapshotForBackend(
    const std::string& backendId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!const_cast<VdrRecordingCacheRepository*>(this)->ensureSchema())
    {
        return false;
    }

    return rebuildBrowseSnapshotFromPersistentCacheLocked(
        normalizeBackendId(backendId));
}

VdrRecordingFolderPage VdrRecordingCacheRepository::folderPageForBackend(
    const std::string& backendId,
    const std::string& folderPath,
    int limit,
    int offset) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    const std::string normalizedBackendId =
        normalizeBackendId(backendId);

    const std::string normalizedFolderPath =
        normalizeFolderPath(folderPath);

    VdrRecordingCacheStatus status =
        statusForBackend(normalizedBackendId);

    VdrRecordingFolderPage page;
    page.backendId = normalizedBackendId;
    page.path = normalizedFolderPath;
    page.parentPath = parentFolderPath(normalizedFolderPath);
    page.cacheState = status.state;
    page.totalCount = status.totalCount;
    page.limit = limit <= 0 ? 50 : std::min(limit, 200);
    page.offset = std::max(0, offset);

    auto snapshot = browseSnapshots_.find(normalizedBackendId);

    if (snapshot == browseSnapshots_.end())
    {
        if (!rebuildBrowseSnapshotFromPersistentCacheLocked(
                normalizedBackendId))
        {
            page.cacheReady = page.totalCount > 0;
            return page;
        }

        snapshot = browseSnapshots_.find(normalizedBackendId);
    }

    if (snapshot == browseSnapshots_.end())
    {
        page.cacheReady = page.totalCount > 0;
        return page;
    }

    if (page.totalCount <= 0)
    {
        page.totalCount = snapshot->second.totalCount;
    }

    if (page.cacheState == "empty" && page.totalCount > 0)
    {
        page.cacheState = "ready";
    }

    page.cacheReady = page.totalCount > 0;

    const auto folder =
        snapshot->second.folders.find(normalizedFolderPath);

    if (folder == snapshot->second.folders.end())
    {
        return page;
    }

    page.folders = folder->second.folders;

    for (auto& child : page.folders)
    {
        const auto childFolder =
            snapshot->second.folders.find(child.path);

        if (childFolder == snapshot->second.folders.end() ||
            !childFolder->second.folders.empty() ||
            childFolder->second.recordings.size() != 1)
        {
            continue;
        }

        child.singleRecordingLeaf = true;
        child.singleRecording =
            childFolder->second.recordings.front();
    }

    page.folderCount =
        static_cast<int>(page.folders.size());

    page.recordingCount =
        static_cast<int>(folder->second.recordings.size());

    const int end =
        std::min(
            page.recordingCount,
            page.offset + page.limit);

    if (page.offset < page.recordingCount)
    {
        for (int index = page.offset; index < end; ++index)
        {
            page.recordings.push_back(
                folder->second.recordings.at(
                    static_cast<std::size_t>(index)));
        }
    }

    return page;
}

void VdrRecordingCacheRepository::storeBrowseSnapshotLocked(
    const std::string& normalizedBackendId,
    const std::vector<VdrRecording>& recordings) const
{
    BrowseBackendSnapshot snapshot;
    snapshot.folders[""] = BrowseFolderSnapshot{};

    const std::vector<VdrRecording> uniqueRecordings =
        uniqueRecordingsByNormalizedPath(recordings);

    snapshot.totalCount =
        static_cast<int>(uniqueRecordings.size());

    std::map<std::string, std::map<std::string, int>> folderCounts;

    for (const VdrRecording& recording : uniqueRecordings)
    {
        const std::string recordingFolder =
            folderPathForRecording(recording);

        snapshot.folders[recordingFolder]
            .recordings.push_back(recording);

        std::vector<std::string> parentSegments;

        for (const std::string& segment : splitPath(recordingFolder))
        {
            const std::string parentPath =
                joinPath(parentSegments);

            parentSegments.push_back(segment);

            const std::string childPath =
                joinPath(parentSegments);

            snapshot.folders[parentPath];
            snapshot.folders[childPath];
            folderCounts[parentPath][childPath] += 1;
        }
    }

    for (const auto& parent : folderCounts)
    {
        auto& folder = snapshot.folders[parent.first];

        for (const auto& child : parent.second)
        {
            VdrRecordingFolderEntry entry;
            entry.path = child.first;
            entry.name = lastSegment(child.first);
            entry.recordingCount = child.second;
            folder.folders.push_back(entry);
        }
    }

    for (auto& folder : snapshot.folders)
    {
        std::sort(
            folder.second.folders.begin(),
            folder.second.folders.end(),
            [](const VdrRecordingFolderEntry& left,
               const VdrRecordingFolderEntry& right)
            {
                return left.name < right.name;
            });

        std::sort(
            folder.second.recordings.begin(),
            folder.second.recordings.end(),
            [](const VdrRecording& left,
               const VdrRecording& right)
            {
                if (left.title != right.title)
                {
                    return left.title < right.title;
                }

                return left.path < right.path;
            });
    }

    browseSnapshots_[normalizedBackendId] =
        std::move(snapshot);
}

bool VdrRecordingCacheRepository::rebuildBrowseSnapshotFromPersistentCacheLocked(
    const std::string& normalizedBackendId) const
{
    const int expectedCount =
        countForBackend(normalizedBackendId);

    const std::vector<VdrRecording> recordings =
        findAllForBackend(normalizedBackendId);

    if (expectedCount > 0 && recordings.empty())
    {
        return false;
    }

    storeBrowseSnapshotLocked(
        normalizedBackendId,
        recordings);

    return true;
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

std::string VdrRecordingCacheRepository::normalizeFolderPath(
    const std::string& folderPath)
{
    return joinPath(splitPath(folderPath));
}

std::string VdrRecordingCacheRepository::parentFolderPath(
    const std::string& folderPath)
{
    std::vector<std::string> segments =
        splitPath(folderPath);

    if (segments.empty())
    {
        return "";
    }

    segments.pop_back();

    return joinPath(segments);
}

std::string VdrRecordingCacheRepository::cacheKeyForRecording(
    const VdrRecording& recording)
{
    return recordingDeduplicationKey(recording);
}

std::string VdrRecordingCacheRepository::folderPathForRecording(
    const VdrRecording& recording)
{
    std::string path =
        recording.path.empty()
            ? recording.backendNativeId
            : recording.path;

    std::vector<std::string> segments =
        splitPath(path);

    removeStorageMountSegments(segments);

    if (!segments.empty())
    {
        if (endsWith(segments.back(), ".rec") ||
            segments.size() > 1)
        {
            segments.pop_back();
        }

        return joinPath(segments);
    }

    std::vector<std::string> titleSegments =
        splitPath(recording.title);

    removeStorageMountSegments(titleSegments);

    if (titleSegments.size() > 1)
    {
        titleSegments.pop_back();
        return joinPath(titleSegments);
    }

    return "";
}

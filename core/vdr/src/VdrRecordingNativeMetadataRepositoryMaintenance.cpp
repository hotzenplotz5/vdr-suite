#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrRecordingNativeMetadataRepositoryInternal.h"
#include "Database.h"
#include <sqlite3.h>

#include <unordered_set>

using namespace vdr_recording_native_repository_detail;

bool VdrRecordingNativeMetadataRepository::removeMissingRecordings(
    const std::string& backendId,
    const std::vector<std::string>& activeRecordingKeys)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    const std::unordered_set<std::string> active(activeRecordingKeys.begin(), activeRecordingKeys.end());
    std::vector<std::string> stale;
    Statement selectStatement(database_.handle(),
        "SELECT recording_key FROM vdr_recording_native_metadata WHERE backend_id = ?;");
    if (!selectStatement.valid() || !bindText(selectStatement.get(), 1, normalizedBackendId)) return false;
    while (sqlite3_step(selectStatement.get()) == SQLITE_ROW)
    {
        const std::string key = columnText(selectStatement.get(), 0);
        if (active.find(key) == active.end()) stale.push_back(key);
    }

    Transaction transaction(database_);
    if (!transaction.active()) return false;
    for (const std::string& key : stale)
    {
        if (!deleteChildren(database_.handle(), normalizedBackendId, key) ||
            !executeDelete(database_.handle(),
                "DELETE FROM vdr_recording_native_metadata WHERE backend_id = ? AND recording_key = ?;",
                normalizedBackendId, key)) return false;
    }
    return transaction.commit();
}

std::vector<std::string>
VdrRecordingNativeMetadataRepository::findDueRecordingKeys(
    const std::string& backendId,
    std::int64_t now,
    int limit) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> keys;
    if (!ensureSchemaLocked() || limit <= 0) return keys;

    Statement statement(database_.handle(),
        "SELECT recording_key FROM vdr_recording_native_metadata "
        "WHERE backend_id = ? AND (next_retry_at=0 OR next_retry_at<=?) AND ("
        "content_state='empty' OR (content_state='found' AND expires_at<=?) OR "
        "(content_state='not_found' AND negative_expires_at<=?)) "
        "ORDER BY next_retry_at ASC, updated_at ASC LIMIT ?;");
    if (!statement.valid() || !bindText(statement.get(), 1, normalizeBackendId(backendId)) ||
        sqlite3_bind_int64(statement.get(), 2, now) != SQLITE_OK ||
        sqlite3_bind_int64(statement.get(), 3, now) != SQLITE_OK ||
        sqlite3_bind_int64(statement.get(), 4, now) != SQLITE_OK ||
        sqlite3_bind_int(statement.get(), 5, limit) != SQLITE_OK) return keys;
    while (sqlite3_step(statement.get()) == SQLITE_ROW) keys.push_back(columnText(statement.get(), 0));
    return keys;
}

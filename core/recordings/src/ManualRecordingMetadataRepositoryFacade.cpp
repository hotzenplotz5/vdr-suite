#include "MetadataRepository.h"

#include "Database.h"

#include <sqlite3.h>

namespace
{
std::string normalizedBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

std::string resolveResourceKey(
    Database& database,
    const std::string& backendId,
    const std::string& suppliedKey)
{
    if (suppliedKey.empty() ||
        !database.tableExists("vdr_recording_cache"))
    {
        return suppliedKey;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT cache_key FROM vdr_recording_cache "
        "WHERE backend_id=? AND (cache_key=? OR backend_native_id=?) "
        "ORDER BY CASE WHEN cache_key=? THEN 0 ELSE 1 END LIMIT 1;";
    if (sqlite3_prepare_v2(
            database.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return suppliedKey;
    }

    const std::string backend = normalizedBackendId(backendId);
    sqlite3_bind_text(statement, 1, backend.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, suppliedKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, suppliedKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, suppliedKey.c_str(), -1, SQLITE_TRANSIENT);

    std::string resolved = suppliedKey;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const unsigned char* value = sqlite3_column_text(statement, 0);
        if (value != nullptr)
        {
            resolved = reinterpret_cast<const char*>(value);
        }
    }
    sqlite3_finalize(statement);
    return resolved;
}
}

bool MetadataRepository::assignManualRecordingMetadata(
    const ManualRecordingMetadataSelection& selection,
    ManualRecordingMetadataAssignment& assigned)
{
    ManualRecordingMetadataSelection resolved = selection;
    resolved.backendId = normalizedBackendId(selection.backendId);
    resolved.resourceKey = resolveResourceKey(
        database_, resolved.backendId, selection.resourceKey);
    ManualRecordingMetadataAssignmentRepository repository(database_);
    return repository.assign(resolved, assigned);
}

bool MetadataRepository::withdrawManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey,
    const std::string& actorRef,
    int expectedRevision,
    ManualRecordingMetadataAssignment& withdrawn)
{
    const std::string backend = normalizedBackendId(backendId);
    ManualRecordingMetadataAssignmentRepository repository(database_);
    return repository.withdraw(
        backend,
        resolveResourceKey(database_, backend, resourceKey),
        actorRef,
        expectedRevision,
        withdrawn);
}

ManualRecordingMetadataAssignment
MetadataRepository::getManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey)
{
    const std::string backend = normalizedBackendId(backendId);
    ManualRecordingMetadataAssignmentRepository repository(database_);
    return repository.findSelected(
        backend,
        resolveResourceKey(database_, backend, resourceKey));
}

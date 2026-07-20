#include "EpgMetadataRepository.h"

#include "Database.h"

#include <sqlite3.h>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : std::string{};
}
}

EpgMetadataRepository::EpgMetadataRepository(Database& database)
    : database_(database)
{
}

bool EpgMetadataRepository::ensureSchema()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool EpgMetadataRepository::ensureSchemaLocked() const
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS epg_event_metadata ("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL,"
        "provider TEXT NOT NULL,"
        "payload TEXT NOT NULL,"
        "resolved_at INTEGER NOT NULL DEFAULT 0,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, channel_id, event_id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_epg_event_metadata_provider "
        "ON epg_event_metadata (backend_id, provider);");
}

bool EpgMetadataRepository::upsert(const EpgMetadataRecord& metadata)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!metadata.valid() ||
        metadata.sourcePayload.empty() ||
        metadata.sourcePayload.size() > 32768 ||
        !ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_event_metadata ("
        "backend_id, channel_id, event_id, provider, payload, resolved_at, "
        "updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id, channel_id, event_id) DO UPDATE SET "
        "provider = excluded.provider, "
        "payload = excluded.payload, "
        "resolved_at = excluded.resolved_at, "
        "updated_at = CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, normalizeBackendId(metadata.backendId)) &&
        bindText(statement, 2, metadata.channelId) &&
        bindText(statement, 3, metadata.eventId) &&
        bindText(statement, 4, metadata.provider) &&
        bindText(statement, 5, metadata.sourcePayload) &&
        sqlite3_bind_int64(statement, 6, metadata.resolvedAt) == SQLITE_OK;

    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

EpgMetadataRecord EpgMetadataRepository::find(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (channelId.empty() || eventId.empty() || !ensureSchemaLocked())
    {
        return {};
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT provider, payload, resolved_at "
        "FROM epg_event_metadata "
        "WHERE backend_id = ? AND channel_id = ? AND event_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return {};
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    const bool bound =
        bindText(statement, 1, normalizedBackendId) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId);

    EpgMetadataRecord metadata;
    if (bound && sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::string provider = columnText(statement, 0);
        const std::string payload = columnText(statement, 1);
        const long long resolvedAt = sqlite3_column_int64(statement, 2);

        if (provider == "tvscraper")
        {
            metadata = parser_.parse(
                payload,
                normalizedBackendId,
                channelId,
                eventId,
                resolvedAt);
        }
    }

    sqlite3_finalize(statement);
    return metadata;
}

bool EpgMetadataRepository::removeForEvent(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM epg_event_metadata "
        "WHERE backend_id = ? AND channel_id = ? AND event_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, normalizeBackendId(backendId)) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId);

    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

std::string EpgMetadataRepository::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

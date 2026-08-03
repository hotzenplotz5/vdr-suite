#include "EpgSeriesArtworkFallbackRepository.h"

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
    return value == nullptr
        ? std::string()
        : reinterpret_cast<const char*>(value);
}

bool eventCacheAllowsWrite(
    Database& database,
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId)
{
    if (!database.tableExists("epg_events"))
    {
        return true;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT 1 FROM epg_events "
        "WHERE backend_id=? AND channel_id=? AND event_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, backendId.empty() ? "default" : backendId) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId);
    const bool found = bound && sqlite3_step(statement) == SQLITE_ROW;
    sqlite3_finalize(statement);
    return found;
}

bool validFallbackReference(const EpgArtworkReference& artwork)
{
    return artwork.valid() &&
        artwork.provider != "none" &&
        artwork.provider != "tvscraper";
}
}

EpgSeriesArtworkFallbackRepository::EpgSeriesArtworkFallbackRepository(
    Database& database)
    : database_(database)
{
}

bool EpgSeriesArtworkFallbackRepository::ensureSchema()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool EpgSeriesArtworkFallbackRepository::ensureSchemaLocked() const
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS epg_series_artwork_fallback ("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL,"
        "provider TEXT NOT NULL,"
        "path TEXT NOT NULL,"
        "width INTEGER NOT NULL,"
        "height INTEGER NOT NULL,"
        "resolved_at INTEGER NOT NULL DEFAULT 0,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, channel_id, event_id),"
        "CHECK (provider <> '' AND provider <> 'none' AND provider <> 'tvscraper'),"
        "CHECK (path <> ''),"
        "CHECK (width > 0 AND height > 0)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_epg_series_artwork_fallback_provider "
        "ON epg_series_artwork_fallback (backend_id, provider);"
    );
}

bool EpgSeriesArtworkFallbackRepository::upsert(
    const EpgArtworkReference& artwork)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validFallbackReference(artwork) ||
        !ensureSchemaLocked() ||
        !eventCacheAllowsWrite(
            database_,
            artwork.backendId,
            artwork.channelId,
            artwork.eventId))
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_series_artwork_fallback ("
        "backend_id,channel_id,event_id,provider,path,width,height,resolved_at,updated_at) "
        "VALUES (?,?,?,?,?,?,?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id,channel_id,event_id) DO UPDATE SET "
        "provider=excluded.provider,path=excluded.path,width=excluded.width,"
        "height=excluded.height,resolved_at=excluded.resolved_at,"
        "updated_at=CURRENT_TIMESTAMP;";
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
        bindText(statement, 1, normalizeBackendId(artwork.backendId)) &&
        bindText(statement, 2, artwork.channelId) &&
        bindText(statement, 3, artwork.eventId) &&
        bindText(statement, 4, artwork.provider) &&
        bindText(statement, 5, artwork.path) &&
        sqlite3_bind_int(statement, 6, artwork.width) == SQLITE_OK &&
        sqlite3_bind_int(statement, 7, artwork.height) == SQLITE_OK &&
        sqlite3_bind_int64(statement, 8, artwork.resolvedAt) == SQLITE_OK;
    const bool stored = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return stored;
}

EpgArtworkReference EpgSeriesArtworkFallbackRepository::find(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    EpgArtworkReference artwork;
    if (channelId.empty() || eventId.empty() || !ensureSchemaLocked())
    {
        return artwork;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT provider,path,width,height,resolved_at "
        "FROM epg_series_artwork_fallback "
        "WHERE backend_id=? AND channel_id=? AND event_id=?;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return artwork;
    }

    artwork.backendId = normalizeBackendId(backendId);
    artwork.channelId = channelId;
    artwork.eventId = eventId;
    const bool bound =
        bindText(statement, 1, artwork.backendId) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId);
    if (bound && sqlite3_step(statement) == SQLITE_ROW)
    {
        artwork.provider = columnText(statement, 0);
        artwork.path = columnText(statement, 1);
        artwork.width = sqlite3_column_int(statement, 2);
        artwork.height = sqlite3_column_int(statement, 3);
        artwork.resolvedAt = sqlite3_column_int64(statement, 4);
    }
    sqlite3_finalize(statement);

    return validFallbackReference(artwork)
        ? artwork
        : EpgArtworkReference{};
}

bool EpgSeriesArtworkFallbackRepository::removeForEvent(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (channelId.empty() || eventId.empty() || !ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM epg_series_artwork_fallback "
        "WHERE backend_id=? AND channel_id=? AND event_id=?;";
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
    const bool removed = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return removed;
}

std::string EpgSeriesArtworkFallbackRepository::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

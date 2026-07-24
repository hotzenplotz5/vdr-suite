#include "EpgArtworkRepository.h"

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

EpgArtworkRepository::EpgArtworkRepository(Database& database)
    : database_(database)
{
}

bool EpgArtworkRepository::ensureSchema()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool EpgArtworkRepository::ensureSchemaLocked() const
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS epg_event_artwork ("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL,"
        "provider TEXT NOT NULL,"
        "path TEXT NOT NULL,"
        "width INTEGER NOT NULL DEFAULT 0,"
        "height INTEGER NOT NULL DEFAULT 0,"
        "resolved_at INTEGER NOT NULL DEFAULT 0,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, channel_id, event_id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_epg_event_artwork_provider "
        "ON epg_event_artwork (backend_id, provider);"
        "CREATE TABLE IF NOT EXISTS epg_scraper_metadata_cache ("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL,"
        "public_json TEXT NOT NULL,"
        "resolved_at INTEGER NOT NULL DEFAULT 0,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, channel_id, event_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS epg_scraper_metadata_images ("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL,"
        "kind TEXT NOT NULL,"
        "image_index INTEGER NOT NULL DEFAULT 0,"
        "provider TEXT NOT NULL,"
        "path TEXT NOT NULL,"
        "width INTEGER NOT NULL DEFAULT 0,"
        "height INTEGER NOT NULL DEFAULT 0,"
        "resolved_at INTEGER NOT NULL DEFAULT 0,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, channel_id, event_id, kind, image_index),"
        "CHECK (kind IN ('preferred','person','gallery'))"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_epg_scraper_metadata_images_event "
        "ON epg_scraper_metadata_images (backend_id, channel_id, event_id);"
    );
}

bool EpgArtworkRepository::upsert(const EpgArtworkReference& artwork)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!artwork.valid() || !ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_event_artwork ("
        "backend_id, channel_id, event_id, provider, path, width, height, "
        "resolved_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id, channel_id, event_id) DO UPDATE SET "
        "provider = excluded.provider, "
        "path = excluded.path, "
        "width = excluded.width, "
        "height = excluded.height, "
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
        bindText(statement, 1, normalizeBackendId(artwork.backendId)) &&
        bindText(statement, 2, artwork.channelId) &&
        bindText(statement, 3, artwork.eventId) &&
        bindText(statement, 4, artwork.provider) &&
        bindText(statement, 5, artwork.path) &&
        sqlite3_bind_int(statement, 6, artwork.width) == SQLITE_OK &&
        sqlite3_bind_int(statement, 7, artwork.height) == SQLITE_OK &&
        sqlite3_bind_int64(statement, 8, artwork.resolvedAt) == SQLITE_OK;

    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

EpgArtworkReference EpgArtworkRepository::find(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    EpgArtworkReference artwork;
    if (!ensureSchemaLocked())
    {
        return artwork;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT provider, path, width, height, resolved_at "
        "FROM epg_event_artwork "
        "WHERE backend_id = ? AND channel_id = ? AND event_id = ?;";

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
    return artwork;
}

bool EpgArtworkRepository::removeForEvent(
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
        "DELETE FROM epg_event_artwork "
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

bool EpgArtworkRepository::upsertMetadataJson(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::string& publicJson,
    long long resolvedAt)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (channelId.empty() || eventId.empty() || publicJson.empty() || !ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_scraper_metadata_cache "
        "(backend_id,channel_id,event_id,public_json,resolved_at,updated_at) "
        "VALUES (?,?,?,?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id,channel_id,event_id) DO UPDATE SET "
        "public_json=excluded.public_json,resolved_at=excluded.resolved_at,updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool bound =
        bindText(statement, 1, normalizeBackendId(backendId)) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId) &&
        bindText(statement, 4, publicJson) &&
        sqlite3_bind_int64(statement, 5, resolvedAt) == SQLITE_OK;
    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

std::string EpgArtworkRepository::findMetadataJson(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return {};

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT public_json FROM epg_scraper_metadata_cache "
        "WHERE backend_id=? AND channel_id=? AND event_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return {};
    }
    const bool bound =
        bindText(statement, 1, normalizeBackendId(backendId)) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId);
    std::string result;
    if (bound && sqlite3_step(statement) == SQLITE_ROW)
    {
        result = columnText(statement, 0);
    }
    sqlite3_finalize(statement);
    return result;
}

bool EpgArtworkRepository::upsertMetadataImage(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::string& kind,
    int imageIndex,
    const EpgArtworkReference& artwork)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (channelId.empty() || eventId.empty() || imageIndex < 0 ||
        (kind != "preferred" && kind != "person" && kind != "gallery") ||
        artwork.provider.empty() || artwork.path.empty() || artwork.width <= 0 ||
        artwork.height <= 0 || !ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_scraper_metadata_images "
        "(backend_id,channel_id,event_id,kind,image_index,provider,path,width,height,resolved_at,updated_at) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id,channel_id,event_id,kind,image_index) DO UPDATE SET "
        "provider=excluded.provider,path=excluded.path,width=excluded.width,height=excluded.height,"
        "resolved_at=excluded.resolved_at,updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool bound =
        bindText(statement, 1, normalizeBackendId(backendId)) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId) &&
        bindText(statement, 4, kind) &&
        sqlite3_bind_int(statement, 5, imageIndex) == SQLITE_OK &&
        bindText(statement, 6, artwork.provider) &&
        bindText(statement, 7, artwork.path) &&
        sqlite3_bind_int(statement, 8, artwork.width) == SQLITE_OK &&
        sqlite3_bind_int(statement, 9, artwork.height) == SQLITE_OK &&
        sqlite3_bind_int64(statement, 10, artwork.resolvedAt) == SQLITE_OK;
    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

EpgArtworkReference EpgArtworkRepository::findMetadataImage(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::string& kind,
    int imageIndex) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    EpgArtworkReference artwork;
    if (!ensureSchemaLocked()) return artwork;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT provider,path,width,height,resolved_at FROM epg_scraper_metadata_images "
        "WHERE backend_id=? AND channel_id=? AND event_id=? AND kind=? AND image_index=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return artwork;
    }
    artwork.backendId = normalizeBackendId(backendId);
    artwork.channelId = channelId;
    artwork.eventId = eventId;
    const bool bound =
        bindText(statement, 1, artwork.backendId) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId) &&
        bindText(statement, 4, kind) &&
        sqlite3_bind_int(statement, 5, imageIndex) == SQLITE_OK;
    if (bound && sqlite3_step(statement) == SQLITE_ROW)
    {
        artwork.provider = columnText(statement, 0);
        artwork.path = columnText(statement, 1);
        artwork.width = sqlite3_column_int(statement, 2);
        artwork.height = sqlite3_column_int(statement, 3);
        artwork.resolvedAt = sqlite3_column_int64(statement, 4);
    }
    sqlite3_finalize(statement);
    return artwork;
}

std::string EpgArtworkRepository::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

#include "EpgArtworkRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <cctype>
#include <vector>

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

    const std::string normalizedBackendId =
        backendId.empty() ? "default" : backendId;
    const bool bound =
        bindText(statement, 1, normalizedBackendId) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId);
    const bool found = bound && sqlite3_step(statement) == SQLITE_ROW;
    sqlite3_finalize(statement);
    return found;
}


std::string foldPersonText(const std::string& value)
{
    std::string folded;
    folded.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const unsigned char byte = static_cast<unsigned char>(value[index]);
        if (byte >= 'A' && byte <= 'Z')
        {
            folded.push_back(static_cast<char>(byte - 'A' + 'a'));
            continue;
        }
        if (byte == 0xc3 && index + 1 < value.size())
        {
            const unsigned char next = static_cast<unsigned char>(value[index + 1]);
            if (next == 0x84 || next == 0xa4) { folded += "ae"; ++index; continue; }
            if (next == 0x96 || next == 0xb6) { folded += "oe"; ++index; continue; }
            if (next == 0x9c || next == 0xbc) { folded += "ue"; ++index; continue; }
            if (next == 0x9f) { folded += "ss"; ++index; continue; }
            if ((next >= 0x80 && next <= 0x96) || (next >= 0x98 && next <= 0x9e))
            {
                folded.push_back(static_cast<char>(0xc3));
                folded.push_back(static_cast<char>(next + 0x20));
                ++index;
                continue;
            }
        }
        folded.push_back(static_cast<char>(byte));
    }
    return folded;
}

const char* personRoleName(EpgScraperPersonRole role)
{
    switch (role)
    {
    case EpgScraperPersonRole::Actor: return "actor";
    case EpgScraperPersonRole::Director: return "director";
    case EpgScraperPersonRole::Writer: return "writer";
    case EpgScraperPersonRole::Producer: return "producer";
    case EpgScraperPersonRole::Moderator: return "moderator";
    case EpgScraperPersonRole::Guest: return "guest";
    case EpgScraperPersonRole::Composer: return "composer";
    case EpgScraperPersonRole::Other: return "other";
    case EpgScraperPersonRole::Unknown: break;
    }
    return "unknown";
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
        "CREATE TABLE IF NOT EXISTS epg_scraper_metadata_people("
        "backend_id TEXT NOT NULL,channel_id TEXT NOT NULL,event_id TEXT NOT NULL,"
        "ordinal INTEGER NOT NULL,role TEXT NOT NULL DEFAULT 'unknown',"
        "name TEXT NOT NULL,name_folded TEXT NOT NULL,"
        "character_name TEXT NOT NULL DEFAULT '',character_name_folded TEXT NOT NULL DEFAULT '',"
        "PRIMARY KEY(backend_id,channel_id,event_id,ordinal));"
        "CREATE INDEX IF NOT EXISTS idx_epg_scraper_metadata_people_name "
        "ON epg_scraper_metadata_people(backend_id,name_folded,channel_id,event_id);"
        "CREATE INDEX IF NOT EXISTS idx_epg_scraper_metadata_people_event "
        "ON epg_scraper_metadata_people(backend_id,channel_id,event_id);"
    );
}

bool EpgArtworkRepository::upsert(const EpgArtworkReference& artwork)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!artwork.valid() ||
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
    if (channelId.empty() ||
        eventId.empty() ||
        publicJson.empty() ||
        !ensureSchemaLocked() ||
        !eventCacheAllowsWrite(database_, backendId, channelId, eventId))
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

bool EpgArtworkRepository::replaceMetadataPeople(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::vector<EpgScraperPerson>& people)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (channelId.empty() || eventId.empty() || !ensureSchemaLocked() ||
        !eventCacheAllowsWrite(database_, backendId, channelId, eventId))
    {
        return false;
    }

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;

    sqlite3_stmt* remove = nullptr;
    const char* removeSql =
        "DELETE FROM epg_scraper_metadata_people "
        "WHERE backend_id=? AND channel_id=? AND event_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), removeSql, -1, &remove, nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return false;
    }
    const std::string normalizedBackend = normalizeBackendId(backendId);
    const bool removeBound =
        bindText(remove, 1, normalizedBackend) &&
        bindText(remove, 2, channelId) &&
        bindText(remove, 3, eventId);
    const bool removed = removeBound && sqlite3_step(remove) == SQLITE_DONE;
    sqlite3_finalize(remove);
    if (!removed)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    sqlite3_stmt* insert = nullptr;
    const char* insertSql =
        "INSERT INTO epg_scraper_metadata_people("
        "backend_id,channel_id,event_id,ordinal,role,name,name_folded,character_name,character_name_folded) "
        "VALUES(?,?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(database_.handle(), insertSql, -1, &insert, nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    bool stored = true;
    for (std::size_t index = 0; index < people.size(); ++index)
    {
        const EpgScraperPerson& person = people[index];
        if (!person.valid()) continue;
        sqlite3_reset(insert);
        sqlite3_clear_bindings(insert);
        stored =
            bindText(insert, 1, normalizedBackend) &&
            bindText(insert, 2, channelId) &&
            bindText(insert, 3, eventId) &&
            sqlite3_bind_int(insert, 4, static_cast<int>(index)) == SQLITE_OK &&
            bindText(insert, 5, personRoleName(person.role)) &&
            bindText(insert, 6, person.name) &&
            bindText(insert, 7, foldPersonText(person.name)) &&
            bindText(insert, 8, person.characterName) &&
            bindText(insert, 9, foldPersonText(person.characterName)) &&
            sqlite3_step(insert) == SQLITE_DONE;
        if (!stored) break;
    }
    sqlite3_finalize(insert);
    if (!stored || !database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }
    return true;
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
        artwork.height <= 0 || !ensureSchemaLocked() ||
        !eventCacheAllowsWrite(database_, backendId, channelId, eventId))
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

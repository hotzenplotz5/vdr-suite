#include "VdrChannelCacheRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <string>
#include <vector>

namespace
{
std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : reinterpret_cast<const char*>(value);
}

void bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT);
}
}

VdrChannelCacheRepository::VdrChannelCacheRepository(Database& database)
    : database_(database)
{
}

bool VdrChannelCacheRepository::ensureSchema()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return database_.execute(
        "CREATE TABLE IF NOT EXISTS vdr_channel_cache("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "channel_number INTEGER NOT NULL DEFAULT 0,"
        "name TEXT NOT NULL DEFAULT '',"
        "provider TEXT NOT NULL DEFAULT '',"
        "group_name TEXT NOT NULL DEFAULT '',"
        "radio INTEGER NOT NULL DEFAULT 0,"
        "encrypted INTEGER NOT NULL DEFAULT 0,"
        "enabled INTEGER NOT NULL DEFAULT 1,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(backend_id,channel_id),"
        "CHECK(radio IN(0,1)),"
        "CHECK(encrypted IN(0,1)),"
        "CHECK(enabled IN(0,1))"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_vdr_channel_cache_backend_number "
        "ON vdr_channel_cache(backend_id,channel_number,channel_id);");
}

bool VdrChannelCacheRepository::replaceChannelsForBackend(
    const std::string& backendId,
    const std::vector<VdrChannel>& channels)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!ensureSchema())
    {
        return false;
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return false;
    }

    sqlite3_stmt* remove = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            "DELETE FROM vdr_channel_cache WHERE backend_id=?;",
            -1,
            &remove,
            nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    bindText(remove, 1, normalizedBackendId);
    const bool removed = sqlite3_step(remove) == SQLITE_DONE;
    sqlite3_finalize(remove);

    if (!removed)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    sqlite3_stmt* insert = nullptr;
    const char* insertSql =
        "INSERT INTO vdr_channel_cache("
        "backend_id,channel_id,channel_number,name,provider,group_name,"
        "radio,encrypted,enabled,updated_at) "
        "VALUES(?,?,?,?,?,?,?,?,?,CURRENT_TIMESTAMP);";

    if (sqlite3_prepare_v2(
            database_.handle(),
            insertSql,
            -1,
            &insert,
            nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    bool stored = true;
    for (const VdrChannel& channel : channels)
    {
        if (channel.id.empty())
        {
            continue;
        }

        sqlite3_reset(insert);
        sqlite3_clear_bindings(insert);
        bindText(insert, 1, normalizedBackendId);
        bindText(insert, 2, channel.id);
        sqlite3_bind_int(insert, 3, channel.number);
        bindText(insert, 4, channel.name);
        bindText(insert, 5, channel.provider);
        bindText(insert, 6, channel.group);
        sqlite3_bind_int(insert, 7, channel.radio ? 1 : 0);
        sqlite3_bind_int(insert, 8, channel.encrypted ? 1 : 0);
        sqlite3_bind_int(insert, 9, channel.enabled ? 1 : 0);

        if (sqlite3_step(insert) != SQLITE_DONE)
        {
            stored = false;
            break;
        }
    }

    sqlite3_finalize(insert);

    if (!stored || !database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    return true;
}

std::vector<VdrChannel> VdrChannelCacheRepository::findAllForBackend(
    const std::string& backendId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<VdrChannel> channels;

    if (!const_cast<VdrChannelCacheRepository*>(this)->ensureSchema())
    {
        return channels;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT channel_id,channel_number,name,provider,group_name,"
        "radio,encrypted,enabled FROM vdr_channel_cache "
        "WHERE backend_id=? ORDER BY channel_number,channel_id;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return channels;
    }

    bindText(statement, 1, normalizeBackendId(backendId));

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        VdrChannel channel;
        channel.id = columnText(statement, 0);
        channel.number = sqlite3_column_int(statement, 1);
        channel.name = columnText(statement, 2);
        channel.provider = columnText(statement, 3);
        channel.group = columnText(statement, 4);
        channel.radio = sqlite3_column_int(statement, 5) != 0;
        channel.encrypted = sqlite3_column_int(statement, 6) != 0;
        channel.enabled = sqlite3_column_int(statement, 7) != 0;
        channels.push_back(std::move(channel));
    }

    sqlite3_finalize(statement);
    return channels;
}

int VdrChannelCacheRepository::countForBackend(
    const std::string& backendId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!const_cast<VdrChannelCacheRepository*>(this)->ensureSchema())
    {
        return 0;
    }

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            "SELECT COUNT(*) FROM vdr_channel_cache WHERE backend_id=?;",
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return 0;
    }

    bindText(statement, 1, normalizeBackendId(backendId));
    const int count = sqlite3_step(statement) == SQLITE_ROW
        ? sqlite3_column_int(statement, 0)
        : 0;
    sqlite3_finalize(statement);
    return count;
}

std::string VdrChannelCacheRepository::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

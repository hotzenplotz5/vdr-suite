#include "EpgEventRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::string columnText(sqlite3_stmt* stmt, int column)
{
    const unsigned char* text = sqlite3_column_text(stmt, column);

    if (!text)
    {
        return {};
    }

    return reinterpret_cast<const char*>(text);
}

void bindText(sqlite3_stmt* stmt, int index, const std::string& value)
{
    sqlite3_bind_text(
        stmt,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT);
}

std::string joinContentDescriptors(
    const std::vector<std::string>& descriptors)
{
    std::ostringstream output;

    for (std::size_t index = 0; index < descriptors.size(); ++index)
    {
        if (index > 0)
        {
            output << '\n';
        }

        output << descriptors.at(index);
    }

    return output.str();
}

std::vector<std::string> splitContentDescriptors(
    const std::string& serialized)
{
    std::vector<std::string> descriptors;
    std::istringstream input(serialized);
    std::string line;

    while (std::getline(input, line))
    {
        if (!line.empty())
        {
            descriptors.push_back(line);
        }
    }

    return descriptors;
}

std::string stableEventId(const VdrEvent& event)
{
    if (!event.id.empty())
    {
        return event.id;
    }

    return event.channelId + ":" + event.startTime + ":" + event.title;
}

VdrEvent readEvent(sqlite3_stmt* stmt)
{
    VdrEvent event;

    event.id = columnText(stmt, 0);
    event.channelId = columnText(stmt, 1);
    event.title = columnText(stmt, 2);
    event.subtitle = columnText(stmt, 3);
    event.description = columnText(stmt, 4);
    event.startTime = columnText(stmt, 5);
    event.endTime = columnText(stmt, 6);
    event.durationSeconds = sqlite3_column_int(stmt, 7);
    event.parentalRating = sqlite3_column_int(stmt, 8);
    event.contentDescriptors = splitContentDescriptors(columnText(stmt, 9));

    return event;
}

std::vector<VdrEvent> readEvents(sqlite3_stmt* stmt)
{
    std::vector<VdrEvent> events;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        events.push_back(readEvent(stmt));
    }

    return events;
}

std::vector<std::string> splitChannelIdsParameter(
    const std::string& channelId)
{
    std::vector<std::string> result;
    std::istringstream input(channelId);
    std::string item;

    while (std::getline(input, item, ','))
    {
        if (!item.empty())
        {
            result.push_back(item);
        }
    }

    return result;
}

std::string sqlPlaceholders(std::size_t count)
{
    std::ostringstream output;

    for (std::size_t index = 0; index < count; ++index)
    {
        if (index > 0)
        {
            output << ", ";
        }

        output << "?";
    }

    return output.str();
}

std::string eventCacheKey(
    const std::string& channelId,
    const std::string& eventId)
{
    return channelId + '\x1f' + eventId;
}

bool executeBound(
    sqlite3* database,
    const std::string& sql,
    const std::vector<std::string>& values)
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database,
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bool bound = true;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        bound = bound &&
            sqlite3_bind_text(
                statement,
                static_cast<int>(index + 1),
                values[index].c_str(),
                -1,
                SQLITE_TRANSIENT) == SQLITE_OK;
    }

    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

bool upsertEvents(
    sqlite3* database,
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_events ("
        "backend_id, channel_id, event_id, title, subtitle, description, "
        "start_time, end_time, duration_seconds, parental_rating, "
        "content_descriptors, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id, channel_id, event_id) DO UPDATE SET "
        "title = excluded.title, "
        "subtitle = excluded.subtitle, "
        "description = excluded.description, "
        "start_time = excluded.start_time, "
        "end_time = excluded.end_time, "
        "duration_seconds = excluded.duration_seconds, "
        "parental_rating = excluded.parental_rating, "
        "content_descriptors = excluded.content_descriptors, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE epg_events.title IS NOT excluded.title "
        "OR epg_events.subtitle IS NOT excluded.subtitle "
        "OR epg_events.description IS NOT excluded.description "
        "OR epg_events.start_time IS NOT excluded.start_time "
        "OR epg_events.end_time IS NOT excluded.end_time "
        "OR epg_events.duration_seconds IS NOT excluded.duration_seconds "
        "OR epg_events.parental_rating IS NOT excluded.parental_rating "
        "OR epg_events.content_descriptors "
        "IS NOT excluded.content_descriptors;";

    if (sqlite3_prepare_v2(
            database,
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    for (const VdrEvent& event : events)
    {
        if (event.channelId.empty())
        {
            continue;
        }

        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);

        bindText(statement, 1, backendId);
        bindText(statement, 2, event.channelId);
        bindText(statement, 3, stableEventId(event));
        bindText(statement, 4, event.title);
        bindText(statement, 5, event.subtitle);
        bindText(statement, 6, event.description);
        bindText(statement, 7, event.startTime);
        bindText(statement, 8, event.endTime);
        sqlite3_bind_int(statement, 9, event.durationSeconds);
        sqlite3_bind_int(statement, 10, event.parentalRating);
        bindText(
            statement,
            11,
            joinContentDescriptors(event.contentDescriptors));

        if (sqlite3_step(statement) != SQLITE_DONE)
        {
            sqlite3_finalize(statement);
            return false;
        }
    }

    return sqlite3_finalize(statement) == SQLITE_OK;
}

bool retireMetadataTarget(
    Database& database,
    const std::string& backendId,
    const EpgEventCacheKey& key)
{
    const std::vector<std::string> values = {
        backendId,
        key.channelId,
        key.eventId};

    if (database.tableExists("suite_metadata_genre_assignments") &&
        database.tableExists("suite_metadata_target_bindings") &&
        !executeBound(
            database.handle(),
            "UPDATE suite_metadata_genre_assignments "
            "SET assignment_state='stale',updated_at=CURRENT_TIMESTAMP "
            "WHERE metadata_target_id IN ("
            "SELECT metadata_target_id FROM suite_metadata_target_bindings "
            "WHERE backend_id=? AND target_type='program-event' "
            "AND channel_id=? AND native_id=?"
            ") AND assignment_state IN ('active','unknown','conflict','missing');",
            values))
    {
        return false;
    }

    if (database.tableExists("suite_metadata_targets") &&
        database.tableExists("suite_metadata_target_bindings") &&
        !executeBound(
            database.handle(),
            "UPDATE suite_metadata_targets "
            "SET lifecycle_state='retired',updated_at=CURRENT_TIMESTAMP "
            "WHERE metadata_target_id IN ("
            "SELECT metadata_target_id FROM suite_metadata_target_bindings "
            "WHERE backend_id=? AND target_type='program-event' "
            "AND channel_id=? AND native_id=?"
            ");",
            values))
    {
        return false;
    }

    return !database.tableExists("suite_metadata_target_bindings") ||
        executeBound(
            database.handle(),
            "UPDATE suite_metadata_target_bindings "
            "SET lifecycle_state='retired',updated_at=CURRENT_TIMESTAMP "
            "WHERE backend_id=? AND target_type='program-event' "
            "AND channel_id=? AND native_id=?;",
            values);
}

bool removeDependentRows(
    Database& database,
    const std::string& backendId,
    const EpgEventCacheKey& key)
{
    const std::vector<std::string> values = {
        backendId,
        key.channelId,
        key.eventId};

    for (const char* table : {
             "epg_event_artwork",
             "epg_scraper_metadata_cache",
             "epg_scraper_metadata_images"})
    {
        if (!database.tableExists(table))
        {
            continue;
        }

        const std::string sql =
            std::string("DELETE FROM ") + table +
            " WHERE backend_id=? AND channel_id=? AND event_id=?;";
        if (!executeBound(database.handle(), sql, values))
        {
            return false;
        }
    }

    return retireMetadataTarget(database, backendId, key) &&
        executeBound(
            database.handle(),
            "DELETE FROM epg_events "
            "WHERE backend_id=? AND channel_id=? AND event_id=?;",
            values);
}


}

EpgEventRepository::EpgEventRepository(Database& database)
    : database_(database)
{
}

bool EpgEventRepository::ensureSchema()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return database_.execute(
        "CREATE TABLE IF NOT EXISTS epg_events ("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL,"
        "title TEXT NOT NULL,"
        "subtitle TEXT NOT NULL DEFAULT '',"
        "description TEXT NOT NULL DEFAULT '',"
        "start_time TEXT NOT NULL,"
        "end_time TEXT NOT NULL,"
        "duration_seconds INTEGER NOT NULL DEFAULT 0,"
        "parental_rating INTEGER NOT NULL DEFAULT 0,"
        "content_descriptors TEXT NOT NULL DEFAULT '',"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, channel_id, event_id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_epg_events_backend_time "
        "ON epg_events (backend_id, start_time, end_time);"
        "CREATE INDEX IF NOT EXISTS idx_epg_events_backend_channel_time "
        "ON epg_events (backend_id, channel_id, start_time, end_time);"
        "CREATE INDEX IF NOT EXISTS idx_epg_events_backend_end_epoch "
        "ON epg_events (backend_id,CAST(end_time AS INTEGER),"
        "CAST(start_time AS INTEGER),channel_id,event_id);"
        "CREATE INDEX IF NOT EXISTS idx_epg_events_backend_title "
        "ON epg_events (backend_id, title);");
}

bool EpgEventRepository::upsertEventsForBackend(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!ensureSchema())
    {
        return false;
    }

    if (events.empty())
    {
        return true;
    }

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return false;
    }

    if (!upsertEvents(
            database_.handle(),
            normalizeBackendId(backendId),
            events))
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

EpgAuthoritativeWindowResult
EpgEventRepository::replaceAuthoritativeWindowForBackend(
    const std::string& backendId,
    const std::string& fromTime,
    const std::string& untilTime,
    const std::vector<std::string>& authoritativeChannelIds,
    const std::vector<VdrEvent>& events)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    EpgAuthoritativeWindowResult result;

    if (!ensureSchema() ||
        fromTime.empty() ||
        untilTime.empty() ||
        authoritativeChannelIds.empty())
    {
        return result;
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    std::set<std::string> authoritativeChannels(
        authoritativeChannelIds.begin(),
        authoritativeChannelIds.end());
    authoritativeChannels.erase("");

    if (authoritativeChannels.empty())
    {
        return result;
    }

    std::set<std::string> incoming;
    for (const VdrEvent& event : events)
    {
        if (authoritativeChannels.find(event.channelId) !=
            authoritativeChannels.end())
        {
            incoming.insert(eventCacheKey(
                event.channelId,
                stableEventId(event)));
        }
    }

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return result;
    }

    std::string selectSql =
        "SELECT channel_id,event_id FROM epg_events "
        "WHERE backend_id=? "
        "AND CAST(end_time AS INTEGER)>CAST(? AS INTEGER) "
        "AND CAST(start_time AS INTEGER)<CAST(? AS INTEGER) "
        "AND channel_id IN (" +
        sqlPlaceholders(authoritativeChannels.size()) +
        ") ORDER BY channel_id,event_id;";

    sqlite3_stmt* selectStatement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            selectSql.c_str(),
            -1,
            &selectStatement,
            nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return result;
    }

    int bindIndex = 1;
    bindText(selectStatement, bindIndex++, normalizedBackendId);
    bindText(selectStatement, bindIndex++, fromTime);
    bindText(selectStatement, bindIndex++, untilTime);
    for (const std::string& channelId : authoritativeChannels)
    {
        bindText(selectStatement, bindIndex++, channelId);
    }

    while (sqlite3_step(selectStatement) == SQLITE_ROW)
    {
        EpgEventCacheKey key;
        key.channelId = columnText(selectStatement, 0);
        key.eventId = columnText(selectStatement, 1);
        if (incoming.find(eventCacheKey(key.channelId, key.eventId)) ==
            incoming.end())
        {
            result.removedEvents.push_back(key);
        }
    }
    sqlite3_finalize(selectStatement);

    if (!upsertEvents(database_.handle(), normalizedBackendId, events))
    {
        database_.execute("ROLLBACK;");
        result.removedEvents.clear();
        return result;
    }

    for (const EpgEventCacheKey& key : result.removedEvents)
    {
        if (!removeDependentRows(database_, normalizedBackendId, key))
        {
            database_.execute("ROLLBACK;");
            result.removedEvents.clear();
            return result;
        }
    }

    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        result.removedEvents.clear();
        return result;
    }

    result.stored = true;
    return result;
}

bool EpgEventRepository::containsEventForBackend(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (channelId.empty() || eventId.empty())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT 1 FROM epg_events "
        "WHERE backend_id=? AND channel_id=? AND event_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bindText(statement, 1, normalizeBackendId(backendId));
    bindText(statement, 2, channelId);
    bindText(statement, 3, eventId);
    const bool found = sqlite3_step(statement) == SQLITE_ROW;
    sqlite3_finalize(statement);
    return found;
}

std::vector<VdrEvent> EpgEventRepository::findNowNextForBackend(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& fromTime,
    int eventLimit) const
{
    return findWindowForBackend(
        backendId,
        channelId,
        fromTime,
        "",
        eventLimit);
}

std::vector<VdrEvent> EpgEventRepository::findWindowForBackend(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& fromTime,
    const std::string& untilTime,
    int eventLimit) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    sqlite3_stmt* stmt = nullptr;

    const std::vector<std::string> channelIds =
        splitChannelIdsParameter(channelId);

    const bool hasChannels = !channelIds.empty();
    const bool hasUntil = !untilTime.empty();
    const bool hasLimit = eventLimit > 0;

    std::string sql =
        "SELECT event_id, channel_id, title, subtitle, description, "
        "start_time, end_time, duration_seconds, parental_rating, "
        "content_descriptors "
        "FROM epg_events "
        "WHERE backend_id = ? "
        "AND end_time > ? ";

    if (hasChannels)
    {
        sql += "AND channel_id IN (";
        sql += sqlPlaceholders(channelIds.size());
        sql += ") ";
    }

    if (hasUntil)
    {
        sql += "AND start_time < ? ";
    }

    sql += "ORDER BY start_time ASC, channel_id ASC, event_id ASC ";

    if (hasLimit)
    {
        sql += "LIMIT ?";
    }

    sql += ";";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return {};
    }

    int bindIndex = 1;
    bindText(stmt, bindIndex++, normalizeBackendId(backendId));
    bindText(stmt, bindIndex++, fromTime);

    for (const std::string& currentChannelId : channelIds)
    {
        bindText(stmt, bindIndex++, currentChannelId);
    }

    if (hasUntil)
    {
        bindText(stmt, bindIndex++, untilTime);
    }

    if (hasLimit)
    {
        sqlite3_bind_int(stmt, bindIndex++, eventLimit);
    }

    std::vector<VdrEvent> events = readEvents(stmt);

    sqlite3_finalize(stmt);

    return events;
}

bool EpgEventRepository::deleteExpiredForBackend(
    const std::string& backendId,
    const std::string& beforeEndTime)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "DELETE FROM epg_events "
        "WHERE backend_id = ? AND end_time < ?;";

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
    bindText(stmt, 2, beforeEndTime);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    return ok;
}

int EpgEventRepository::countForBackend(
    const std::string& backendId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT COUNT(*) FROM epg_events WHERE backend_id = ?;";

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

std::string EpgEventRepository::normalizeBackendId(
    const std::string& backendId)
{
    if (backendId.empty())
    {
        return "default";
    }

    return backendId;
}

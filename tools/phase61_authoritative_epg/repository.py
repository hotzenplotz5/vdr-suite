from .common import replace_once

# ---------------------------------------------------------------------------
# EPG repository: authoritative window replacement, never title/time identity.
# ---------------------------------------------------------------------------

replace_once(
    "core/vdr/include/EpgEventRepository.h",
    '#include <mutex>\n#include <string>\n#include <vector>\n',
    '#include <cstddef>\n#include <mutex>\n#include <string>\n#include <vector>\n'
)

replace_once(
    "core/vdr/include/EpgEventRepository.h",
    '''class Database;

class EpgEventRepository
''',
    '''class Database;

struct EpgEventCacheKey
{
    std::string channelId;
    std::string eventId;
};

struct EpgAuthoritativeWindowResult
{
    bool stored = false;
    std::vector<EpgEventCacheKey> removedEvents;
};

class EpgEventRepository
'''
)

replace_once(
    "core/vdr/include/EpgEventRepository.h",
    '''    bool upsertEventsForBackend(
        const std::string& backendId,
        const std::vector<VdrEvent>& events);

    std::vector<VdrEvent> findNowNextForBackend(
''',
    '''    bool upsertEventsForBackend(
        const std::string& backendId,
        const std::vector<VdrEvent>& events);

    EpgAuthoritativeWindowResult replaceAuthoritativeWindowForBackend(
        const std::string& backendId,
        const std::string& fromTime,
        const std::string& untilTime,
        const std::vector<std::string>& authoritativeChannelIds,
        const std::vector<VdrEvent>& events);

    bool containsEventForBackend(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const;

    std::vector<VdrEvent> findNowNextForBackend(
'''
)

replace_once(
    "core/vdr/src/EpgEventRepository.cpp",
    '#include <sstream>\n#include <string>\n#include <vector>\n',
    '#include <set>\n#include <sstream>\n#include <string>\n#include <vector>\n'
)

repository_helpers = r'''
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
        "updated_at = CURRENT_TIMESTAMP;";

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
'''

replace_once(
    "core/vdr/src/EpgEventRepository.cpp",
    '''std::string sqlPlaceholders(std::size_t count)
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

}
''',
    '''std::string sqlPlaceholders(std::size_t count)
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
''' + repository_helpers + '''

}
'''
)

old_upsert = r'''bool EpgEventRepository::upsertEventsForBackend(
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

    sqlite3_stmt* stmt = nullptr;

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

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        sqlite3_finalize(stmt);
        return false;
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);

    for (const VdrEvent& event : events)
    {
        if (event.channelId.empty())
        {
            continue;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        bindText(stmt, 1, normalizedBackendId);
        bindText(stmt, 2, event.channelId);
        bindText(stmt, 3, stableEventId(event));
        bindText(stmt, 4, event.title);
        bindText(stmt, 5, event.subtitle);
        bindText(stmt, 6, event.description);
        bindText(stmt, 7, event.startTime);
        bindText(stmt, 8, event.endTime);
        sqlite3_bind_int(stmt, 9, event.durationSeconds);
        sqlite3_bind_int(stmt, 10, event.parentalRating);
        bindText(stmt, 11, joinContentDescriptors(event.contentDescriptors));

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            database_.execute("ROLLBACK;");
            return false;
        }
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
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
'''

new_upsert = r'''bool EpgEventRepository::upsertEventsForBackend(
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
            result.removedEvents.push_back(std::move(key));
        }
    }
    sqlite3_finalize(selectStatement);

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        result.removedEvents.clear();
        return result;
    }

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
'''

replace_once(
    "core/vdr/src/EpgEventRepository.cpp",
    old_upsert,
    new_upsert
)


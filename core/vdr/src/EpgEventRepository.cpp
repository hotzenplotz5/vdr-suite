#include "EpgEventRepository.h"

#include "Database.h"

#include <sqlite3.h>

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
}

EpgEventRepository::EpgEventRepository(Database& database)
    : database_(database)
{
}

bool EpgEventRepository::ensureSchema()
{
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
        "CREATE INDEX IF NOT EXISTS idx_epg_events_backend_title "
        "ON epg_events (backend_id, title);");
}

bool EpgEventRepository::upsertEventsForBackend(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    if (!ensureSchema())
    {
        return false;
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
            return false;
        }
    }

    sqlite3_finalize(stmt);

    return true;
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
    sqlite3_stmt* stmt = nullptr;

    const bool hasChannel = !channelId.empty();
    const bool hasUntil = !untilTime.empty();
    const bool hasLimit = eventLimit > 0;

    std::string sql =
        "SELECT event_id, channel_id, title, subtitle, description, "
        "start_time, end_time, duration_seconds, parental_rating, "
        "content_descriptors "
        "FROM epg_events "
        "WHERE backend_id = ? "
        "AND end_time > ? ";

    if (hasChannel)
    {
        sql += "AND channel_id = ? ";
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

    if (hasChannel)
    {
        bindText(stmt, bindIndex++, channelId);
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

#include "EpgPersonIndexRepository.h"

#include "Database.h"

#include <sqlite3.h>

namespace
{

bool bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

}

bool EpgPersonIndexRepository::removeForEvent(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked() || channelId.empty() || eventId.empty())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM suite_metadata_epg_person_index "
        "WHERE backend_id=? AND channel_id=? AND event_id=?;";

    const bool ok = sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) == SQLITE_OK &&
        bindText(statement, 1, normalizeBackendId(backendId)) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId) &&
        sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);
    return ok;
}

#include "MediaTranscodeBackendSettingsRepository.h"

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
}

bool MediaTranscodeBackendSettingsRepository::ensureSchema() const
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_media_transcode_settings ("
        "backend_id TEXT PRIMARY KEY,"
        "video_encoder_mode TEXT NOT NULL,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "CHECK(video_encoder_mode IN ('auto','software','vaapi'))"
        ");");
}

bool MediaTranscodeBackendSettingsRepository::readManagedMode(
    const std::string& backendId,
    std::string& mode) const
{
    mode.clear();
    if (!database_.isOpen() || !ensureSchema()) return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT video_encoder_mode FROM backend_media_transcode_settings "
        "WHERE backend_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound = bindText(statement, 1, backendId);
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    if (step == SQLITE_ROW) mode = columnText(statement, 0);
    sqlite3_finalize(statement);
    return step == SQLITE_ROW || step == SQLITE_DONE;
}

bool MediaTranscodeBackendSettingsRepository::storeManagedMode(
    const std::string& backendId,
    const std::string& mode) const
{
    if (!database_.isOpen() || !ensureSchema()) return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO backend_media_transcode_settings "
        "(backend_id,video_encoder_mode,updated_at) "
        "VALUES (?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "video_encoder_mode=excluded.video_encoder_mode,"
        "updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool stored =
        bindText(statement, 1, backendId) &&
        bindText(statement, 2, mode) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return stored;
}

bool MediaTranscodeBackendSettingsRepository::clearManagedMode(
    const std::string& backendId) const
{
    if (!database_.isOpen() || !ensureSchema()) return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM backend_media_transcode_settings WHERE backend_id=?;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool removed =
        bindText(statement, 1, backendId) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return removed;
}

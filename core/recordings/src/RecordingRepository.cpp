#include "RecordingRepository.h"
#include "Database.h"

#include <sqlite3.h>

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

    Recording readRecording(sqlite3_stmt* stmt)
    {
        Recording recording;

        recording.id = sqlite3_column_int(stmt, 0);
        recording.title = columnText(stmt, 1);
        recording.subtitle = columnText(stmt, 2);
        recording.description = columnText(stmt, 3);
        recording.channel = columnText(stmt, 4);
        recording.recordingPath = columnText(stmt, 5);
        recording.recordingFormat = columnText(stmt, 6);

        return recording;
    }
}

RecordingRepository::RecordingRepository(Database& database)
    : database_(database)
{
}

std::vector<std::string> RecordingRepository::getRecordingTitles()
{
    std::vector<std::string> result;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT title FROM recordings ORDER BY title;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result.emplace_back(columnText(stmt, 0));
    }

    sqlite3_finalize(stmt);

    return result;
}

std::vector<Recording> RecordingRepository::getAllRecordings()
{
    std::vector<Recording> result;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT id, title, subtitle, description, channel, recording_path, recording_format "
        "FROM recordings "
        "ORDER BY title;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result.push_back(readRecording(stmt));
    }

    sqlite3_finalize(stmt);

    return result;
}

std::optional<Recording> RecordingRepository::getRecordingById(int id)
{
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT id, title, subtitle, description, channel, recording_path, recording_format "
        "FROM recordings "
        "WHERE id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    std::optional<Recording> result;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = readRecording(stmt);
    }

    sqlite3_finalize(stmt);

    return result;
}

std::vector<Recording> RecordingRepository::findByTitle(
    const std::string& title)
{
    std::vector<Recording> result;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT id, title, subtitle, description, channel, "
        "recording_path, recording_format "
        "FROM recordings "
        "WHERE title LIKE ? "
        "ORDER BY title;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    std::string pattern =
        "%" + title + "%";

    sqlite3_bind_text(
        stmt,
        1,
        pattern.c_str(),
        -1,
        SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result.push_back(readRecording(stmt));
    }

    sqlite3_finalize(stmt);

    return result;
}

#include "RecordingRepository.h"
#include "Database.h"

#include <sqlite3.h>

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
        const unsigned char* title =
            sqlite3_column_text(stmt, 0);

        if (title)
        {
            result.emplace_back(
                reinterpret_cast<const char*>(title));
        }
    }

    sqlite3_finalize(stmt);

    return result;
}

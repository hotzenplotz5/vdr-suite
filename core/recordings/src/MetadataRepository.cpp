#include "MetadataRepository.h"
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
}

MetadataRepository::MetadataRepository(Database& database)
    : database_(database)
{
}

std::vector<Metadata> MetadataRepository::getAllMetadata()
{
    std::vector<Metadata> result;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT id, recording_id, media_type, title, original_title, year, "
        "season_number, episode_number, genre, description, source, external_id "
        "FROM metadata "
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
        Metadata metadata;

        metadata.id = sqlite3_column_int(stmt, 0);
        metadata.recordingId = sqlite3_column_int(stmt, 1);
        metadata.mediaType = columnText(stmt, 2);
        metadata.title = columnText(stmt, 3);
        metadata.originalTitle = columnText(stmt, 4);
        metadata.year = sqlite3_column_int(stmt, 5);
        metadata.seasonNumber = sqlite3_column_int(stmt, 6);
        metadata.episodeNumber = sqlite3_column_int(stmt, 7);
        metadata.genre = columnText(stmt, 8);
        metadata.description = columnText(stmt, 9);
        metadata.source = columnText(stmt, 10);
        metadata.externalId = columnText(stmt, 11);

        result.push_back(metadata);
    }

    sqlite3_finalize(stmt);

    return result;
}

#include "RecordingSeriesHierarchyOverrideRepository.h"

#include <algorithm>
#include <sqlite3.h>

namespace
{
bool validScopedKey(
    const std::string& value,
    std::size_t maximum)
{
    if (value.empty() || value.size() > maximum)
    {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character) {
            return character >= 0x20U &&
                character != 0x7fU;
        });
}

bool validGroupType(const std::string& value)
{
    return value == "season" ||
        value == "special" ||
        value == "custom";
}

bool validLabel(const std::string& value)
{
    return value.size() <= 160U &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character) {
                return character >= 0x20U &&
                    character != 0x7fU;
            });
}

RecordingSeriesHierarchyOverride rowValue(
    sqlite3_stmt* statement)
{
    RecordingSeriesHierarchyOverride value;

    value.available = true;

    const auto textColumn =
        [statement](int column) -> std::string {
            const unsigned char* raw =
                sqlite3_column_text(statement, column);

            return raw == nullptr
                ? std::string{}
                : reinterpret_cast<const char*>(raw);
        };

    value.backendId = textColumn(0);
    value.recordingKey = textColumn(1);
    value.groupType = textColumn(2);
    value.seasonNumber =
        sqlite3_column_int(statement, 3);
    value.groupLabel = textColumn(4);
    value.sortOrder =
        sqlite3_column_int(statement, 5);
    value.episodeStart =
        sqlite3_column_int(statement, 6);
    value.episodeEnd =
        sqlite3_column_int(statement, 7);
    value.revision =
        sqlite3_column_int(statement, 8);

    return value;
}

RecordingSeriesHierarchyOverrideMutationResult
errorResult(
    int statusCode,
    const std::string& errorCode,
    const std::string& message)
{
    RecordingSeriesHierarchyOverrideMutationResult result;

    result.statusCode = statusCode;
    result.errorCode = errorCode;
    result.message = message;

    return result;
}

bool bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT) == SQLITE_OK;
}
}

RecordingSeriesHierarchyOverrideRepository::
RecordingSeriesHierarchyOverrideRepository(
    Database& database)
    : database_(database)
{
}

bool RecordingSeriesHierarchyOverrideRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS "
        "recording_series_hierarchy_overrides ("
        "backend_id TEXT NOT NULL,"
        "recording_key TEXT NOT NULL,"
        "group_type TEXT NOT NULL,"
        "season_number INTEGER NOT NULL DEFAULT 0,"
        "group_label TEXT NOT NULL DEFAULT '',"
        "sort_order INTEGER NOT NULL DEFAULT 0,"
        "episode_start INTEGER NOT NULL DEFAULT 0,"
        "episode_end INTEGER NOT NULL DEFAULT 0,"
        "revision INTEGER NOT NULL DEFAULT 1,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(backend_id,recording_key)"
        ");"
    );
}

RecordingSeriesHierarchyOverride
RecordingSeriesHierarchyOverrideRepository::get(
    const std::string& backendId,
    const std::string& recordingKey) const
{
    RecordingSeriesHierarchyOverride result;

    if (!validScopedKey(backendId, 128U) ||
        !validScopedKey(recordingKey, 4096U))
    {
        return result;
    }

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            "SELECT "
            "backend_id,"
            "recording_key,"
            "group_type,"
            "season_number,"
            "group_label,"
            "sort_order,"
            "episode_start,"
            "episode_end,"
            "revision "
            "FROM recording_series_hierarchy_overrides "
            "WHERE backend_id=? AND recording_key=?;",
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    if (!bindText(statement, 1, backendId) ||
        !bindText(statement, 2, recordingKey))
    {
        sqlite3_finalize(statement);
        return result;
    }

    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        result = rowValue(statement);
    }

    sqlite3_finalize(statement);
    return result;
}

RecordingSeriesHierarchyOverrideMutationResult
RecordingSeriesHierarchyOverrideRepository::set(
    const RecordingSeriesHierarchyOverrideMutation& mutation)
{
    if (!validScopedKey(mutation.backendId, 128U))
    {
        return errorResult(
            400,
            "invalid_backend_id",
            "Backend id is invalid");
    }

    if (!validScopedKey(mutation.recordingKey, 4096U))
    {
        return errorResult(
            400,
            "invalid_recording_key",
            "Recording key is invalid");
    }

    if (!validGroupType(mutation.groupType))
    {
        return errorResult(
            400,
            "invalid_group_type",
            "Group type must be season, special or custom");
    }

    if (!validLabel(mutation.groupLabel))
    {
        return errorResult(
            400,
            "invalid_group_label",
            "Group label is invalid");
    }

    if (mutation.groupType == "season" &&
        mutation.seasonNumber <= 0)
    {
        return errorResult(
            400,
            "invalid_season_number",
            "Season overrides require a positive season number");
    }

    if (mutation.groupType != "season" &&
        mutation.groupLabel.empty())
    {
        return errorResult(
            400,
            "group_label_required",
            "Special and custom groups require a label");
    }

    if (mutation.episodeStart < 0 ||
        mutation.episodeEnd < 0 ||
        (mutation.episodeEnd > 0 &&
         mutation.episodeStart <= 0) ||
        (mutation.episodeEnd > 0 &&
         mutation.episodeEnd < mutation.episodeStart))
    {
        return errorResult(
            400,
            "invalid_episode_range",
            "Episode range is invalid");
    }

    const RecordingSeriesHierarchyOverride current =
        get(
            mutation.backendId,
            mutation.recordingKey);

    if (mutation.expectedRevision > 0 &&
        (!current.available ||
         current.revision != mutation.expectedRevision))
    {
        return errorResult(
            409,
            "revision_conflict",
            "Hierarchy override revision changed");
    }

    const int revision =
        current.available
            ? current.revision + 1
            : 1;

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            "INSERT INTO recording_series_hierarchy_overrides("
            "backend_id,"
            "recording_key,"
            "group_type,"
            "season_number,"
            "group_label,"
            "sort_order,"
            "episode_start,"
            "episode_end,"
            "revision,"
            "updated_at"
            ") VALUES(?,?,?,?,?,?,?,?,?,CURRENT_TIMESTAMP) "
            "ON CONFLICT(backend_id,recording_key) DO UPDATE SET "
            "group_type=excluded.group_type,"
            "season_number=excluded.season_number,"
            "group_label=excluded.group_label,"
            "sort_order=excluded.sort_order,"
            "episode_start=excluded.episode_start,"
            "episode_end=excluded.episode_end,"
            "revision=excluded.revision,"
            "updated_at=CURRENT_TIMESTAMP;",
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return errorResult(
            500,
            "hierarchy_override_write_failed",
            "Hierarchy override could not be prepared");
    }

    const bool bound =
        bindText(statement, 1, mutation.backendId) &&
        bindText(statement, 2, mutation.recordingKey) &&
        bindText(statement, 3, mutation.groupType) &&
        sqlite3_bind_int(
            statement,
            4,
            mutation.groupType == "season"
                ? mutation.seasonNumber
                : 0) == SQLITE_OK &&
        bindText(
            statement,
            5,
            mutation.groupLabel) &&
        sqlite3_bind_int(
            statement,
            6,
            mutation.sortOrder) == SQLITE_OK &&
        sqlite3_bind_int(
            statement,
            7,
            mutation.episodeStart) == SQLITE_OK &&
        sqlite3_bind_int(
            statement,
            8,
            mutation.episodeEnd) == SQLITE_OK &&
        sqlite3_bind_int(
            statement,
            9,
            revision) == SQLITE_OK;

    if (!bound)
    {
        sqlite3_finalize(statement);

        return errorResult(
            500,
            "hierarchy_override_bind_failed",
            "Hierarchy override could not be bound");
    }

    const int step = sqlite3_step(statement);

    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return errorResult(
            500,
            "hierarchy_override_write_failed",
            "Hierarchy override could not be stored");
    }

    RecordingSeriesHierarchyOverrideMutationResult result;

    result.success = true;
    result.statusCode = 200;
    result.value = get(
        mutation.backendId,
        mutation.recordingKey);

    return result;
}

RecordingSeriesHierarchyOverrideMutationResult
RecordingSeriesHierarchyOverrideRepository::clear(
    const std::string& backendId,
    const std::string& recordingKey,
    int expectedRevision)
{
    if (!validScopedKey(backendId, 128U) ||
        !validScopedKey(recordingKey, 4096U))
    {
        return errorResult(
            400,
            "invalid_hierarchy_override_scope",
            "Hierarchy override scope is invalid");
    }

    const RecordingSeriesHierarchyOverride current =
        get(
            backendId,
            recordingKey);

    if (expectedRevision > 0 &&
        (!current.available ||
         current.revision != expectedRevision))
    {
        return errorResult(
            409,
            "revision_conflict",
            "Hierarchy override revision changed");
    }

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            "DELETE FROM recording_series_hierarchy_overrides "
            "WHERE backend_id=? AND recording_key=?;",
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return errorResult(
            500,
            "hierarchy_override_delete_failed",
            "Hierarchy override could not be prepared for deletion");
    }

    if (!bindText(statement, 1, backendId) ||
        !bindText(statement, 2, recordingKey))
    {
        sqlite3_finalize(statement);

        return errorResult(
            500,
            "hierarchy_override_bind_failed",
            "Hierarchy override could not be bound");
    }

    const int step = sqlite3_step(statement);

    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return errorResult(
            500,
            "hierarchy_override_delete_failed",
            "Hierarchy override could not be deleted");
    }

    RecordingSeriesHierarchyOverrideMutationResult result;

    result.success = true;
    result.statusCode = 200;

    return result;
}

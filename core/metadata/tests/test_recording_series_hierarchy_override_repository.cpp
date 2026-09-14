#include "Database.h"
#include "RecordingSeriesHierarchyOverrideRepository.h"

#include <cassert>
#include <filesystem>
#include <sqlite3.h>
#include <string>

namespace
{
bool columnExists(
    Database& database,
    const std::string& table,
    const std::string& column)
{
    sqlite3_stmt* statement = nullptr;

    const std::string sql =
        "PRAGMA table_info(" + table + ");";

    assert(sqlite3_prepare_v2(
        database.handle(),
        sql.c_str(),
        -1,
        &statement,
        nullptr) == SQLITE_OK);

    bool found = false;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const unsigned char* value =
            sqlite3_column_text(statement, 1);

        if (value != nullptr &&
            column ==
                reinterpret_cast<const char*>(value))
        {
            found = true;
            break;
        }
    }

    sqlite3_finalize(statement);
    return found;
}
}

int main()
{
    const auto root =
        std::filesystem::temp_directory_path() /
        "vdr-suite-series-hierarchy-override-test";

    std::error_code error;
    std::filesystem::remove_all(root, error);

    Database database;
    assert(
        database.open(
            (root.string() + ".db")));

    RecordingSeriesHierarchyOverrideRepository repository(
        database);

    assert(repository.ensureSchema());

    assert(columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "backend_id"));

    assert(columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "recording_key"));

    assert(columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "group_type"));

    assert(columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "season_number"));

    assert(columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "group_label"));

    assert(columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "episode_start"));

    assert(columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "episode_end"));

    assert(columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "revision"));

    // Native metadata remains completely separate.
    assert(!columnExists(
        database,
        "recording_series_hierarchy_overrides",
        "provider_id"));

    const std::string recording =
        "Battlestar_Galactica/Pilot";

    assert(
        !repository.get(
            "default",
            recording).available);

    RecordingSeriesHierarchyOverrideMutation pilot;
    pilot.backendId = "default";
    pilot.recordingKey = recording;
    pilot.groupType = "special";
    pilot.groupLabel = "Pilot / Miniserie";
    pilot.sortOrder = -100;
    pilot.episodeStart = 1;
    pilot.episodeEnd = 2;

    const auto pilotStored =
        repository.set(pilot);

    assert(pilotStored.success);
    assert(pilotStored.statusCode == 200);
    assert(pilotStored.value.available);
    assert(
        pilotStored.value.groupType ==
        "special");
    assert(
        pilotStored.value.groupLabel ==
        "Pilot / Miniserie");
    assert(
        pilotStored.value.seasonNumber == 0);
    assert(
        pilotStored.value.episodeStart == 1);
    assert(
        pilotStored.value.episodeEnd == 2);
    assert(
        pilotStored.value.revision == 1);

    // A normal Season assignment is also supported.
    RecordingSeriesHierarchyOverrideMutation season;
    season.backendId = "default";
    season.recordingKey =
        "Battlestar_Galactica/Regular_Episode";
    season.groupType = "season";
    season.seasonNumber = 2;

    const auto seasonStored =
        repository.set(season);

    assert(seasonStored.success);
    assert(
        seasonStored.value.groupType ==
        "season");
    assert(
        seasonStored.value.seasonNumber == 2);

    // Razor / TV movies can live in their own semantic group.
    RecordingSeriesHierarchyOverrideMutation movies;
    movies.backendId = "default";
    movies.recordingKey =
        "Battlestar_Galactica/Razor";
    movies.groupType = "special";
    movies.groupLabel = "TV-Filme / Specials";
    movies.sortOrder = 1000;

    const auto moviesStored =
        repository.set(movies);

    assert(moviesStored.success);
    assert(
        moviesStored.value.groupLabel ==
        "TV-Filme / Specials");

    // Backend scope must not leak.
    assert(
        !repository.get(
            "house-b",
            recording).available);

    // Revision fencing.
    RecordingSeriesHierarchyOverrideMutation replacePilot =
        pilot;

    replacePilot.groupLabel =
        "Miniserie";
    replacePilot.expectedRevision = 1;

    const auto replaced =
        repository.set(replacePilot);

    assert(replaced.success);
    assert(
        replaced.value.revision == 2);

    replacePilot.expectedRevision = 1;

    const auto conflict =
        repository.set(replacePilot);

    assert(!conflict.success);
    assert(conflict.statusCode == 409);
    assert(
        conflict.errorCode ==
        "revision_conflict");

    // Invalid grouping is fail-closed.
    RecordingSeriesHierarchyOverrideMutation invalid =
        pilot;

    invalid.recordingKey = "invalid";
    invalid.groupType = "episode";

    const auto invalidResult =
        repository.set(invalid);

    assert(!invalidResult.success);
    assert(invalidResult.statusCode == 400);

    // Invalid season assignment is fail-closed.
    invalid = pilot;
    invalid.recordingKey = "invalid-season";
    invalid.groupType = "season";
    invalid.groupLabel.clear();
    invalid.seasonNumber = 0;

    const auto invalidSeason =
        repository.set(invalid);

    assert(!invalidSeason.success);
    assert(
        invalidSeason.errorCode ==
        "invalid_season_number");

    // Clear restores native ownership.
    const auto cleared =
        repository.clear(
            "default",
            recording,
            2);

    assert(cleared.success);
    assert(
        !repository.get(
            "default",
            recording).available);

    std::filesystem::remove(
        root.string() + ".db",
        error);

    return 0;
}

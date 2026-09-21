#include "Database.h"
#include "ManualRecordingMetadataAssignmentRepository.h"
#include "MetadataIdentity.h"

#include <sqlite3.h>

#include <cassert>
#include <string>

namespace
{
int scalarInt(Database& database, const std::string& sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(), sql.c_str(), -1, &statement, nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const int value = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return value;
}

ManualRecordingMetadataSelection movieSelection()
{
    ManualRecordingMetadataSelection selection;
    selection.backendId = "living-room";
    selection.resourceKey = "recording-cache-key-1";
    selection.providerId = "tmdb";
    selection.externalNamespace = "movie";
    selection.externalId = "13";
    selection.mediaType = "movie";
    selection.title = "Forrest Gump";
    selection.originalTitle = "Forrest Gump";
    selection.overview = "A selected movie candidate.";
    selection.releaseDate = "1994-07-06";
    selection.posterReference = "/saHP97rTPS5eLmrLQEcANmKrsFl.jpg";
    selection.actorRef = "user:test-admin";
    return selection;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    ManualRecordingMetadataAssignmentRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.ensureSchema());
    assert(database.tableExists("suite_metadata_manual_assignment_values"));
    assert(database.tableExists("suite_metadata_manual_assignment_withdrawals"));

    {
        ManualRecordingMetadataSelection invalid = movieSelection();
        invalid.posterReference = "https://image.tmdb.org/poster.jpg";
        ManualRecordingMetadataAssignment ignored;
        assert(!repository.assign(invalid, ignored));
        assert(!ignored.found);
    }

    ManualRecordingMetadataAssignment first;
    assert(repository.assign(movieSelection(), first));
    assert(first.found);
    assert(first.backendId == "living-room");
    assert(first.resourceKey == "recording-cache-key-1");
    assert(first.providerId == "tmdb");
    assert(first.externalNamespace == "movie");
    assert(first.externalId == "13");
    assert(first.mediaType == "movie");
    assert(first.title == "Forrest Gump");
    assert(first.posterReference == "/saHP97rTPS5eLmrLQEcANmKrsFl.jpg");
    assert(first.revision == 1);
    assert(first.relationshipLocked);
    assert(MetadataTargetId::isValidValue(first.metadataTargetId));
    assert(MetadataAssignmentId::isValidValue(first.metadataAssignmentId));
    assert(MetadataEntityId::isValidValue(first.metadataEntityId));

    const ManualRecordingMetadataAssignment loaded = repository.findSelected(
        "living-room", "recording-cache-key-1");
    assert(loaded.found);
    assert(loaded.metadataAssignmentId == first.metadataAssignmentId);
    assert(loaded.actorRef == "user:test-admin");

    {
        ManualRecordingMetadataSelection stale = movieSelection();
        stale.externalId = "999";
        stale.expectedRevision = 9;
        ManualRecordingMetadataAssignment ignored;
        assert(!repository.assign(stale, ignored));
        assert(repository.findSelected(
            "living-room", "recording-cache-key-1").externalId == "13");
    }

    ManualRecordingMetadataSelection episode = movieSelection();
    episode.providerId = "tvdb";
    episode.externalNamespace = "series";
    episode.externalId = "77811";
    episode.mediaType = "episode";
    episode.title = "Sherlock";
    episode.originalTitle = "Sherlock";
    episode.overview = "A manually selected episode.";
    episode.releaseDate = "2010-07-25";
    episode.posterReference = "/sherlock-season.jpg";
    episode.seasonNumber = 1;
    episode.episodeNumber = 1;
    episode.expectedRevision = first.revision;

    ManualRecordingMetadataAssignment second;
    assert(repository.assign(episode, second));
    assert(second.found);
    assert(second.revision == 2);
    assert(second.mediaType == "episode");
    assert(second.seasonNumber == 1);
    assert(second.episodeNumber == 1);
    assert(second.metadataAssignmentId != first.metadataAssignmentId);
    assert(second.metadataEntityId != first.metadataEntityId);

    assert(scalarInt(
        database,
        "SELECT COUNT(*) FROM suite_metadata_assignments "
        "WHERE assignment_state='selected' AND manual_assignment=1;") == 1);
    assert(scalarInt(
        database,
        "SELECT COUNT(*) FROM suite_metadata_assignments "
        "WHERE assignment_state='superseded' AND manual_assignment=1;") == 1);
    assert(scalarInt(database, "SELECT COUNT(*) FROM suite_metadata_evidence;") == 2);
    assert(scalarInt(
        database,
        "SELECT COUNT(*) FROM suite_metadata_assignment_evidence "
        "WHERE evidence_role='manual-override';") == 2);

    ManualRecordingMetadataAssignment ignoredWithdrawal;
    assert(!repository.withdraw(
        "living-room",
        "recording-cache-key-1",
        "user:test-admin",
        1,
        ignoredWithdrawal));

    ManualRecordingMetadataAssignment withdrawn;
    assert(repository.withdraw(
        "living-room",
        "recording-cache-key-1",
        "user:test-admin",
        second.revision,
        withdrawn));
    assert(withdrawn.metadataAssignmentId == second.metadataAssignmentId);
    assert(!withdrawn.relationshipLocked);
    assert(!repository.findSelected(
        "living-room", "recording-cache-key-1").found);
    assert(scalarInt(
        database,
        "SELECT COUNT(*) FROM suite_metadata_manual_assignment_withdrawals;") == 1);
    assert(scalarInt(
        database,
        "SELECT COUNT(*) FROM suite_metadata_assignments "
        "WHERE assignment_state='withdrawn' AND manual_assignment=1;") == 1);

    database.close();
    return 0;
}

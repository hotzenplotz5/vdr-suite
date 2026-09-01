#include "Database.h"
#include "VdrRecordingNativeMetadataRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>

namespace
{
VdrRecordingNativeArtwork artwork(const std::string& path, int width, int height, const std::string& orientation = {})
{
    VdrRecordingNativeArtwork value; value.available = true; value.provider = "tvscraper"; value.path = path; value.width = width; value.height = height; value.orientation = orientation; return value;
}

VdrRecordingNativeMetadata foundMetadata(const std::string& key)
{
    VdrRecordingNativeMetadata metadata;
    metadata.availability = VdrRecordingNativeMetadataAvailability::Found;
    metadata.schema = 1; metadata.found = true; metadata.reason = "none"; metadata.provider = "tvscraper";
    metadata.recordingIdentitySchema = 1; metadata.recordingKey = key; metadata.mediaType = "movie"; metadata.providerId = 13;
    metadata.runtimeMinutes = 142; metadata.popularity = 99.5; metadata.voteAverage = 8.8; metadata.voteCount = 1234;
    metadata.title = "Forrest Gump"; metadata.originalTitle = "Forrest Gump"; metadata.tagline = "Life is like a box of chocolates";
    metadata.overview = "Forrest describes his life."; metadata.releaseDate = "1994-07-06"; metadata.imdbId = "tt0109830"; metadata.status = "Released";
    metadata.genres = {"Drama", "Comedy"}; metadata.productionCountries = {"US"};
    metadata.preferredArtwork = artwork("movies/13/poster.jpg", 780, 1170);
    VdrRecordingNativePerson tom; tom.role = "actor"; tom.name = "Tom Hanks"; tom.characterName = "Forrest Gump"; tom.image = artwork("actors/tom-hanks.jpg", 300, 450); metadata.people.push_back(tom);
    VdrRecordingNativePerson robert; robert.role = "director"; robert.name = "Robert Zemeckis"; robert.image.provider = "none"; metadata.people.push_back(robert);
    metadata.images.push_back(artwork("movies/13/backdrop.jpg", 1280, 720, "landscape"));
    return metadata;
}

VdrRecordingNativeMetadata negativeMetadata(const std::string& key)
{
    VdrRecordingNativeMetadata metadata;
    metadata.availability = VdrRecordingNativeMetadataAvailability::NotFound;
    metadata.schema = 1;
    metadata.found = false;
    metadata.reason = "provider-no-match";
    metadata.provider = "none";
    metadata.recordingIdentitySchema = 1;
    metadata.recordingKey = key;
    metadata.mediaType = "none";
    metadata.preferredArtwork.provider = "none";
    return metadata;
}

void assertFoundRecord(const VdrRecordingNativeMetadataRecord& record, const std::string& nativeId, const std::string& key)
{
    assert(record.exists()); assert(record.backendId == "default"); assert(record.backendNativeId == nativeId); assert(record.recordingKey == key);
    assert(record.contentState == "found"); assert(record.lastAttemptState == "found"); assert(record.resolvedAt == 1000); assert(record.expiresAt == 2000); assert(record.retryCount == 0);
    assert(record.metadata.found); assert(record.metadata.title == "Forrest Gump"); assert(record.metadata.imdbId == "tt0109830");
    assert(record.metadata.genres.size() == 2); assert(record.metadata.productionCountries.size() == 1); assert(record.metadata.people.size() == 2);
    assert(record.metadata.people[0].name == "Tom Hanks"); assert(record.metadata.people[0].characterName == "Forrest Gump");
    assert(record.metadata.people[0].image.path == "actors/tom-hanks.jpg"); assert(record.metadata.preferredArtwork.path == "movies/13/poster.jpg");
    assert(record.metadata.images.size() == 1); assert(record.metadata.images[0].orientation == "landscape");
}
}

int main()
{
    const char* path = "/tmp/test_vdr_recording_native_metadata_repository.db";
    std::remove(path);
    const std::string key = "c94d0eb9958a85079f81f059a436003c";
    const std::string nativeId = "/srv/vdr/video/Forrest_Gump/2026-07-20.20.15.1-0.rec";
    const std::string migrationKey = "11111111111111111111111111111111";
    const std::string migrationNativeId = "/srv/vdr/video/Serien/Band_of_Brothers/01_Currahee/2016-03-29.00.55.1-0.rec";

    {
        Database database; assert(database.open(path)); VdrRecordingNativeMetadataRepository repository(database);
        assert(repository.ensureSchema()); assert(database.tableExists("vdr_recording_native_metadata"));
        assert(database.tableExists("vdr_recording_native_person")); assert(database.tableExists("vdr_recording_native_artwork"));
        assert(database.tableExists("vdr_recording_native_metadata_migration"));
        assert(repository.storeResolution("", nativeId, foundMetadata(key), 1000, 2000, 0));
        assertFoundRecord(repository.find("default", key), nativeId, key);

        VdrRecordingNativePersonSearchQuery query; query.name = "hAnKs"; query.limit = 20;
        const auto byName = repository.searchPeople("default", query); assert(byName.totalCount == 1); assert(byName.entries.size() == 1);
        assert(byName.entries[0].normalizedName == "tom-hanks"); assert(byName.entries[0].role == "actor");
        query = {}; query.characterName = "forrest"; const auto byCharacter = repository.searchPeople("default", query);
        assert(byCharacter.totalCount == 1); assert(byCharacter.entries[0].name == "Tom Hanks");
        query = {}; query.role = "director"; const auto byRole = repository.searchPeople("default", query);
        assert(byRole.totalCount == 1); assert(byRole.entries[0].name == "Robert Zemeckis");
        query = {}; query.limit = 1; query.offset = 1; const auto page = repository.searchPeople("default", query);
        assert(page.totalCount == 2); assert(page.entries.size() == 1);
    }

    {
        Database database; assert(database.open(path)); VdrRecordingNativeMetadataRepository repository(database);
        assertFoundRecord(repository.find("default", key), nativeId, key);
        assert(repository.recordFailure("default", nativeId, key, VdrRecordingNativeMetadataAvailability::TransportError, "temporary SVDRP failure", 2, 1500));
        const auto stale = repository.find("default", key); assert(stale.contentState == "found"); assert(stale.lastAttemptState == "transport_error");
        assert(stale.metadata.title == "Forrest Gump"); assert(stale.retryCount == 2); assert(stale.nextRetryAt == 1500); assert(stale.lastError == "temporary SVDRP failure");
        assert(repository.findDueRecordingKeys("default", 1400, 10).empty());
        const auto dueAfter = repository.findDueRecordingKeys("default", 2100, 10); assert(dueAfter.size() == 1); assert(dueAfter[0] == key);

        assert(repository.storeResolution("default", nativeId, negativeMetadata(key), 2200, 0, 3000));
        const auto missing = repository.find("default", key); assert(missing.contentState == "not_found"); assert(!missing.metadata.found);
        assert(missing.metadata.people.empty()); assert(missing.metadata.images.empty()); assert(missing.negativeExpiresAt == 3000);
        assert(repository.searchPeople("default", {}).totalCount == 0);

        assert(repository.storeResolution("living-room", nativeId, foundMetadata(key), 1000, 2000, 0));
        assert(repository.find("living-room", key).exists()); assert(repository.removeMissingRecordings("default", {}));
        assert(!repository.find("default", key).exists()); assert(repository.find("living-room", key).exists());

        assert(repository.storeResolution(
            "migration",
            migrationNativeId,
            negativeMetadata(migrationKey),
            4000,
            0,
            999999));
        const auto cachedFalseNegative = repository.find("migration", migrationKey);
        assert(cachedFalseNegative.contentState == "not_found");
        assert(cachedFalseNegative.negativeExpiresAt == 999999);
        assert(database.execute(
            "DELETE FROM vdr_recording_native_metadata_migration "
            "WHERE name = 'tvscraper-signed-series-provider-id-v1';"));
    }

    {
        Database database; assert(database.open(path)); VdrRecordingNativeMetadataRepository repository(database);
        const auto migrated = repository.find("migration", migrationKey);
        assert(migrated.exists());
        assert(migrated.contentState == "not_found");
        assert(migrated.metadata.reason == "provider-no-match");
        assert(migrated.negativeExpiresAt == 0);
    }

    std::remove(path);
    std::cout << "test_vdr_recording_native_metadata_repository passed\n";
    return 0;
}

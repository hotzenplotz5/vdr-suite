#include "Database.h"
#include "PersonQuery.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrRecordingNativePersonSearchService.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace
{

VdrRecording makeRecording(
    const std::string& id,
    const std::string& backendNativeId,
    const std::string& title)
{
    VdrRecording recording;
    recording.id = id;
    recording.backendNativeId = backendNativeId;
    recording.title = title;
    recording.path = backendNativeId;
    recording.startTime = "1780000000";
    recording.durationSeconds = 7200;
    recording.sizeMb = 4096;
    return recording;
}

VdrRecordingNativeMetadata makeMetadata(
    const std::string& recordingKey,
    int providerId,
    const std::string& title,
    const std::string& characterName)
{
    VdrRecordingNativeMetadata metadata;
    metadata.availability =
        VdrRecordingNativeMetadataAvailability::Found;
    metadata.schema = 1;
    metadata.found = true;
    metadata.reason = "none";
    metadata.provider = "tvscraper";
    metadata.recordingIdentitySchema = 1;
    metadata.recordingKey = recordingKey;
    metadata.mediaType = "movie";
    metadata.providerId = providerId;
    metadata.title = title;
    metadata.originalTitle = title;

    VdrRecordingNativePerson person;
    person.role = "actor";
    person.name = "Tom Hanks";
    person.characterName = characterName;
    metadata.people.push_back(person);

    return metadata;
}

bool containsTitle(
    const RecordingPersonSearchResult& result,
    const std::string& title)
{
    for (const RecordingPersonSearchMatch& match : result.matches())
    {
        if (match.recording().title == title)
        {
            return true;
        }
    }

    return false;
}

}

int main()
{
    const char* databasePath =
        "/tmp/test_vdr_recording_native_person_search_service.db";

    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));

    VdrRecordingCacheRepository recordingCacheRepository(database);
    VdrRecordingNativeMetadataRepository metadataRepository(database);

    assert(recordingCacheRepository.ensureSchema());
    assert(metadataRepository.ensureSchema());

    const std::string infernoNativeId =
        "/srv/vdr/video/Thriller/Inferno/2026-05-21.20.38.1-0.rec";
    const std::string sullyNativeId =
        "/srv/vdr/video/Drama/Sully/2026-06-01.20.15.1-0.rec";
    const std::string castAwayNativeId =
        "/srv/vdr/video/Filme/Cast_Away/2026-06-02.20.15.1-0.rec";

    assert(recordingCacheRepository.replaceRecordingsForBackend(
        "default",
        {
            makeRecording("inferno", infernoNativeId, "Thriller/Inferno"),
            makeRecording("sully", sullyNativeId, "Drama/Sully")
        }));

    assert(recordingCacheRepository.replaceRecordingsForBackend(
        "remote",
        {
            makeRecording("cast-away", castAwayNativeId, "Filme/Cast Away")
        }));

    assert(metadataRepository.storeResolution(
        "default",
        infernoNativeId,
        makeMetadata(
            "11111111111111111111111111111111",
            207932,
            "Inferno",
            "Robert Langdon"),
        1000,
        2000,
        0));

    assert(metadataRepository.storeResolution(
        "default",
        sullyNativeId,
        makeMetadata(
            "22222222222222222222222222222222",
            363676,
            "Sully",
            "Chesley Sullenberger"),
        1000,
        2000,
        0));

    assert(metadataRepository.storeResolution(
        "remote",
        castAwayNativeId,
        makeMetadata(
            "33333333333333333333333333333333",
            8358,
            "Cast Away",
            "Chuck Noland"),
        1000,
        2000,
        0));

    VdrRecordingNativePersonSearchService service(
        metadataRepository,
        recordingCacheRepository);

    const RecordingPersonSearchResult byName =
        service.search(
            "default",
            PersonQuery::byName("Tom Hanks"),
            20,
            0);

    assert(byName.totalCount() == 2);
    assert(byName.returnedCount() == 2);
    assert(containsTitle(byName, "Thriller/Inferno"));
    assert(containsTitle(byName, "Drama/Sully"));

    const RecordingPersonSearchResult byNormalizedName =
        service.search(
            "default",
            PersonQuery::byNormalizedName("tom-hanks"),
            20,
            0);

    assert(byNormalizedName.totalCount() == 2);

    const RecordingPersonSearchResult byCharacter =
        service.search(
            "default",
            PersonQuery::byCharacterName("Langdon"),
            20,
            0);

    assert(byCharacter.totalCount() == 1);
    assert(byCharacter.returnedCount() == 1);
    assert(byCharacter.matches().front().recording().title ==
        "Thriller/Inferno");
    assert(byCharacter.matches().front().person().characterName() ==
        "Robert Langdon");

    const RecordingPersonSearchResult byRole =
        service.search(
            "default",
            PersonQuery::byRole(PersonRole::Actor),
            20,
            0);

    assert(byRole.totalCount() == 2);

    const RecordingPersonSearchResult page =
        service.search(
            "default",
            PersonQuery::byName("Hanks"),
            1,
            1);

    assert(page.totalCount() == 2);
    assert(page.returnedCount() == 1);
    assert(page.limit() == 1);
    assert(page.offset() == 1);

    const RecordingPersonSearchResult remote =
        service.search(
            "remote",
            PersonQuery::byName("Tom Hanks"),
            20,
            0);

    assert(remote.totalCount() == 1);
    assert(remote.returnedCount() == 1);
    assert(remote.matches().front().recording().backendId == "remote");
    assert(remote.matches().front().recording().title == "Filme/Cast Away");

    PersonQuery wrongSource = PersonQuery::byName("Tom Hanks");
    wrongSource.withSource(ContentClassificationSource::Tmdb);

    assert(service.search("default", wrongSource, 20, 0).empty());

    assert(service.search(
        "default",
        PersonQuery::byProviderReference("tmdb:31"),
        20,
        0).empty());

    database.close();
    std::remove(databasePath);

    std::cout
        << "test_vdr_recording_native_person_search_service passed"
        << std::endl;

    return 0;
}

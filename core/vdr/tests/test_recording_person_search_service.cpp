#include "RecordingPersonSearchService.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

VdrRecording makeRecording(
    const std::string& id,
    const std::string& title)
{
    VdrRecording recording;

    recording.id = id;
    recording.backendId = "default";
    recording.title = title;
    recording.path = "/" + title;
    recording.backendNativeId = recording.path;
    recording.startTime = "1780000000";
    recording.durationSeconds = 8000;
    recording.sizeMb = 7000;

    return recording;
}

Person makePerson(
    PersonRole role,
    const std::string& originalName,
    const std::string& normalizedName,
    const std::string& characterName)
{
    return Person::withCharacterName(
        ContentClassificationSource::Tvscraper,
        role,
        originalName,
        normalizedName,
        characterName);
}

}

int main()
{
    VdrRecording forrestGump =
        makeRecording(
            "1",
            "Movies/Forrest Gump");

    forrestGump.persons.add(
        makePerson(
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Forrest Gump"));
    forrestGump.persons.add(
        makePerson(
            PersonRole::Director,
            "Robert Zemeckis",
            "robert-zemeckis",
            ""));

    VdrRecording castAway =
        makeRecording(
            "2",
            "Movies/Cast Away");

    castAway.persons.add(
        makePerson(
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Chuck Noland"));
    castAway.persons.add(
        makePerson(
            PersonRole::Composer,
            "Alan Silvestri",
            "alan-silvestri",
            ""));

    VdrRecording savingPrivateRyan =
        makeRecording(
            "3",
            "Movies/Saving Private Ryan");

    savingPrivateRyan.persons.add(
        makePerson(
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Captain Miller"));
    savingPrivateRyan.persons.add(
        makePerson(
            PersonRole::Director,
            "Steven Spielberg",
            "steven-spielberg",
            ""));

    std::vector<VdrRecording> recordings = {
        forrestGump,
        castAway,
        savingPrivateRyan
    };

    RecordingPersonSearchService service;

    RecordingPersonSearchResult emptyQueryResult =
        service.search(
            recordings,
            PersonQuery::createEmpty(),
            0,
            0);

    assert(emptyQueryResult.totalCount() == 6);
    assert(emptyQueryResult.returnedCount() == 6);

    RecordingPersonSearchResult actorResult =
        service.search(
            recordings,
            PersonQuery::createEmpty().withRole(PersonRole::Actor),
            10,
            0);

    assert(actorResult.totalCount() == 3);
    assert(actorResult.returnedCount() == 3);
    assert(actorResult.matches().at(0).recording().id == "1");
    assert(actorResult.matches().at(0).person().normalizedName() == "tom-hanks");
    assert(actorResult.matches().at(0).person().characterName() == "Forrest Gump");

    RecordingPersonSearchResult directorResult =
        service.search(
            recordings,
            PersonQuery::createEmpty().withRole(PersonRole::Director),
            10,
            0);

    assert(directorResult.totalCount() == 2);
    assert(directorResult.returnedCount() == 2);
    assert(directorResult.matches().at(0).person().normalizedName() == "robert-zemeckis");
    assert(directorResult.matches().at(1).person().normalizedName() == "steven-spielberg");

    RecordingPersonSearchResult composerResult =
        service.search(
            recordings,
            PersonQuery::createEmpty().withRole(PersonRole::Composer),
            10,
            0);

    assert(composerResult.totalCount() == 1);
    assert(composerResult.matches().at(0).person().normalizedName() == "alan-silvestri");

    RecordingPersonSearchResult normalizedNameResult =
        service.search(
            recordings,
            PersonQuery::createEmpty().withNormalizedName("tom-hanks"),
            10,
            0);

    assert(normalizedNameResult.totalCount() == 3);
    assert(normalizedNameResult.returnedCount() == 3);

    RecordingPersonSearchResult pagedResult =
        service.search(
            recordings,
            PersonQuery::createEmpty().withNormalizedName("tom-hanks"),
            1,
            1);

    assert(pagedResult.totalCount() == 3);
    assert(pagedResult.returnedCount() == 1);
    assert(pagedResult.limit() == 1);
    assert(pagedResult.offset() == 1);
    assert(pagedResult.matches().at(0).recording().id == "2");

    RecordingPersonSearchResult negativePagingResult =
        service.search(
            recordings,
            PersonQuery::createEmpty().withNormalizedName("tom-hanks"),
            -5,
            -2);

    assert(negativePagingResult.totalCount() == 3);
    assert(negativePagingResult.returnedCount() == 3);
    assert(negativePagingResult.limit() == 0);
    assert(negativePagingResult.offset() == 0);

    std::cout << "test_recording_person_search_service passed" << std::endl;
    return 0;
}

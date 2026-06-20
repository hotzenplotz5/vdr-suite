#include "RecordingPersonSearchController.h"

#include "RecordingPersonSearchResultJsonSerializer.h"
#include "RecordingPersonSearchService.h"

#include <cassert>
#include <iostream>
#include <string>
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
    return Person::withProviderReference(
        ContentClassificationSource::Tvscraper,
        role,
        originalName,
        normalizedName,
        characterName,
        95,
        "tvscraper:example");
}

std::vector<VdrRecording> makeRecordings()
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

    return {
        forrestGump,
        castAway
    };
}

}

int main()
{
    RecordingPersonSearchService searchService;
    RecordingPersonSearchResultJsonSerializer jsonSerializer;

    RecordingPersonSearchController controller(
        searchService,
        jsonSerializer);

    const std::vector<VdrRecording> recordings =
        makeRecordings();

    ApiResponse allResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "",
            "",
            "",
            10,
            0);

    assert(allResponse.statusCode == 200);
    assert(allResponse.contentType == "application/json");
    assert(allResponse.body.find("\"totalCount\":4") != std::string::npos);
    assert(allResponse.body.find("\"returnedCount\":4") != std::string::npos);
    assert(allResponse.body.find("\"matches\":[") != std::string::npos);

    ApiResponse actorResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "actor",
            "",
            "",
            10,
            0);

    assert(actorResponse.statusCode == 200);
    assert(actorResponse.body.find("\"totalCount\":2") != std::string::npos);
    assert(actorResponse.body.find("\"role\":\"actor\"") != std::string::npos);
    assert(actorResponse.body.find("\"normalizedName\":\"tom-hanks\"") != std::string::npos);
    assert(actorResponse.body.find("\"recording\":{") != std::string::npos);

    ApiResponse directorResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "director",
            "",
            "",
            10,
            0);

    assert(directorResponse.statusCode == 200);
    assert(directorResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(directorResponse.body.find("\"normalizedName\":\"robert-zemeckis\"") != std::string::npos);

    ApiResponse composerResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "composer",
            "",
            "",
            10,
            0);

    assert(composerResponse.statusCode == 200);
    assert(composerResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(composerResponse.body.find("\"normalizedName\":\"alan-silvestri\"") != std::string::npos);

    ApiResponse normalizedNameResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "tom-hanks",
            "",
            "",
            "",
            "",
            10,
            0);

    assert(normalizedNameResponse.statusCode == 200);
    assert(normalizedNameResponse.body.find("\"totalCount\":2") != std::string::npos);
    assert(normalizedNameResponse.body.find("\"title\":\"Movies/Forrest Gump\"") != std::string::npos);
    assert(normalizedNameResponse.body.find("\"title\":\"Movies/Cast Away\"") != std::string::npos);

    ApiResponse sourceResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "",
            "tvscraper",
            "",
            10,
            0);

    assert(sourceResponse.statusCode == 200);
    assert(sourceResponse.body.find("\"totalCount\":4") != std::string::npos);
    assert(sourceResponse.body.find("\"source\":\"tvscraper\"") != std::string::npos);

    ApiResponse providerResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "",
            "",
            "tvscraper:example",
            10,
            0);

    assert(providerResponse.statusCode == 200);
    assert(providerResponse.body.find("\"totalCount\":4") != std::string::npos);

    ApiResponse pagedResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "",
            "",
            "",
            1,
            1);

    assert(pagedResponse.statusCode == 200);
    assert(pagedResponse.body.find("\"totalCount\":4") != std::string::npos);
    assert(pagedResponse.body.find("\"returnedCount\":1") != std::string::npos);
    assert(pagedResponse.body.find("\"limit\":1") != std::string::npos);
    assert(pagedResponse.body.find("\"offset\":1") != std::string::npos);

    ApiResponse invalidRoleResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "hero",
            "",
            "",
            10,
            0);

    assert(invalidRoleResponse.statusCode == 400);
    assert(invalidRoleResponse.body.find("invalid person role") != std::string::npos);

    ApiResponse invalidSourceResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "",
            "provider-x",
            "",
            10,
            0);

    assert(invalidSourceResponse.statusCode == 400);
    assert(invalidSourceResponse.body.find("invalid person source") != std::string::npos);

    ApiResponse negativeLimitResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "",
            "",
            "",
            -1,
            0);

    assert(negativeLimitResponse.statusCode == 400);
    assert(negativeLimitResponse.body.find("limit must not be negative") != std::string::npos);

    ApiResponse negativeOffsetResponse =
        controller.searchRecordingPersons(
            recordings,
            "",
            "",
            "",
            "",
            "",
            "",
            10,
            -1);

    assert(negativeOffsetResponse.statusCode == 400);
    assert(negativeOffsetResponse.body.find("offset must not be negative") != std::string::npos);

    std::cout << "test_recording_person_search_controller passed" << std::endl;
    return 0;
}

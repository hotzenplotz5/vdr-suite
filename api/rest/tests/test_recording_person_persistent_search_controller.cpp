#include "RecordingPersonSearchController.h"

#include "Person.h"
#include "RecordingPersonSearchResultJsonSerializer.h"
#include "RecordingPersonSearchService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>

int main()
{
    RecordingPersonSearchService fallbackSearchService;
    RecordingPersonSearchResultJsonSerializer jsonSerializer;

    bool persistentSearchCalled = false;
    std::string searchedBackendId;
    int identityLookups = 0;

    RecordingPersonSearchController controller(
        fallbackSearchService,
        jsonSerializer,
        [&](const std::string& backendId,
            const PersonQuery& query,
            int limit,
            int offset)
        {
            persistentSearchCalled = true;
            searchedBackendId = backendId;

            assert(query.hasName());
            assert(query.name() == "Tom Hanks");
            assert(query.hasRole());
            assert(query.role() == PersonRole::Actor);
            assert(query.hasSource());
            assert(query.source() == ContentClassificationSource::Tvscraper);
            assert(limit == 20);
            assert(offset == 0);

            VdrRecording recording;
            recording.id = "inferno";
            recording.backendId = backendId;
            recording.backendNativeId =
                "/srv/vdr/video/Thriller/Inferno/2026-05-21.20.38.1-0.rec";
            recording.title = "Thriller/Inferno";
            recording.path = recording.backendNativeId;

            RecordingPersonSearchMatch match(
                std::move(recording),
                Person::withCharacterName(
                    ContentClassificationSource::Tvscraper,
                    PersonRole::Actor,
                    "Tom Hanks",
                    "tom-hanks",
                    "Robert Langdon"));

            return RecordingPersonSearchResult::from(
                {std::move(match)},
                1,
                limit,
                offset);
        },
        [&](const std::string& backend, const std::string& native)
        {
            ++identityLookups;
            assert(backend == "remote");
            assert(native == "/srv/vdr/video/Thriller/Inferno/2026-05-21.20.38.1-0.rec");
            return std::string("rec_0123456789abcdef0123456789abcdef");
        });

    assert(controller.usesPersistentSearch());

    const ApiResponse response =
        controller.searchRecordingPersons(
            "remote",
            {},
            "Tom Hanks",
            "",
            "",
            "actor",
            "tvscraper",
            "",
            20,
            0);

    assert(persistentSearchCalled);
    assert(identityLookups == 1);
    assert(response.body.find("\"publicRecordingId\":\"rec_0123456789abcdef0123456789abcdef\"") != std::string::npos);
    assert(searchedBackendId == "remote");
    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"totalCount\":1") != std::string::npos);
    assert(response.body.find("\"backendId\":\"remote\"") != std::string::npos);
    assert(response.body.find("\"title\":\"Thriller/Inferno\"") != std::string::npos);
    assert(response.body.find("\"originalName\":\"Tom Hanks\"") != std::string::npos);
    assert(response.body.find("\"characterName\":\"Robert Langdon\"") != std::string::npos);

    std::cout
        << "test_recording_person_persistent_search_controller passed"
        << std::endl;

    return 0;
}

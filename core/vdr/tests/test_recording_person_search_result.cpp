#include "RecordingPersonSearchResult.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

VdrRecording makeRecording(
    const std::string& id,
    const std::string& title,
    const std::string& path)
{
    VdrRecording recording;

    recording.id = id;
    recording.backendId = "default";
    recording.title = title;
    recording.path = path;
    recording.backendNativeId = path;
    recording.startTime = "1780000000";
    recording.durationSeconds = 8000;
    recording.sizeMb = 7000;

    return recording;
}

Person makeActor(
    const std::string& originalName,
    const std::string& normalizedName,
    const std::string& characterName)
{
    return Person::withCharacterName(
        ContentClassificationSource::Tvscraper,
        PersonRole::Actor,
        originalName,
        normalizedName,
        characterName);
}

}

int main()
{
    RecordingPersonSearchResult empty =
        RecordingPersonSearchResult::empty(
            25,
            0);

    assert(empty.empty());
    assert(empty.totalCount() == 0);
    assert(empty.returnedCount() == 0);
    assert(empty.limit() == 25);
    assert(empty.offset() == 0);
    assert(empty.matches().empty());

    VdrRecording forrestGump =
        makeRecording(
            "1",
            "Movies/Forrest Gump",
            "/Movies/Forrest_Gump/2026-01-01.20.15.1-0.rec");

    VdrRecording castAway =
        makeRecording(
            "2",
            "Movies/Cast Away",
            "/Movies/Cast_Away/2026-01-02.20.15.1-0.rec");

    Person tomHanksForrest =
        makeActor(
            "Tom Hanks",
            "tom-hanks",
            "Forrest Gump");

    Person tomHanksChuck =
        makeActor(
            "Tom Hanks",
            "tom-hanks",
            "Chuck Noland");

    std::vector<RecordingPersonSearchMatch> matches;
    matches.emplace_back(
        forrestGump,
        tomHanksForrest);
    matches.emplace_back(
        castAway,
        tomHanksChuck);

    RecordingPersonSearchResult result =
        RecordingPersonSearchResult::from(
            matches,
            7,
            2,
            4);

    assert(!result.empty());
    assert(result.totalCount() == 7);
    assert(result.returnedCount() == 2);
    assert(result.limit() == 2);
    assert(result.offset() == 4);

    assert(result.matches().at(0).recording().id == "1");
    assert(result.matches().at(0).recording().title == "Movies/Forrest Gump");
    assert(result.matches().at(0).recording().path == "/Movies/Forrest_Gump/2026-01-01.20.15.1-0.rec");
    assert(result.matches().at(0).person().originalName() == "Tom Hanks");
    assert(result.matches().at(0).person().normalizedName() == "tom-hanks");
    assert(result.matches().at(0).person().characterName() == "Forrest Gump");
    assert(result.matches().at(0).person().role() == PersonRole::Actor);

    assert(result.matches().at(1).recording().id == "2");
    assert(result.matches().at(1).recording().title == "Movies/Cast Away");
    assert(result.matches().at(1).person().characterName() == "Chuck Noland");

    std::cout << "test_recording_person_search_result passed" << std::endl;
    return 0;
}

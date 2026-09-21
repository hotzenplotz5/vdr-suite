#include "RecordingPersonSearchResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

VdrRecording makeRecording(
    const std::string& id,
    const std::string& backendId,
    const std::string& title,
    const std::string& path)
{
    VdrRecording recording;

    recording.id = id;
    recording.backendId = backendId;
    recording.title = title;
    recording.path = path;
    recording.backendNativeId = path;
    recording.startTime = "1780000000";
    recording.durationSeconds = 8000;
    recording.sizeMb = 7000;

    recording.metadata.provider.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    recording.metadata.provider.contentKind =
        VdrRecordingContentKind::Movie;
    recording.metadata.provider.title = "Forrest Gump";
    recording.metadata.provider.tagline = "Das Leben ist wie eine Schachtel Pralinen.";
    recording.metadata.provider.overview = "Eine vorhandene Aufnahme mit vollständigen Metadaten.";

    VdrRecordingArtworkRef artwork;
    artwork.kind = VdrRecordingArtworkKind::Poster;
    artwork.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    artwork.reference = "/var/cache/vdr/poster.jpg";
    artwork.width = 500;
    artwork.height = 750;
    artwork.temporary = false;
    recording.metadata.artwork.push_back(artwork);

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

}

int main()
{
    RecordingPersonSearchResultJsonSerializer serializer;

    std::string emptyJson =
        serializer.serialize(
            RecordingPersonSearchResult::empty(
                25,
                0));

    assert(emptyJson.find("\"totalCount\":0") != std::string::npos);
    assert(emptyJson.find("\"returnedCount\":0") != std::string::npos);
    assert(emptyJson.find("\"limit\":25") != std::string::npos);
    assert(emptyJson.find("\"offset\":0") != std::string::npos);
    assert(emptyJson.find("\"matches\":[]") != std::string::npos);

    std::vector<RecordingPersonSearchMatch> matches;
    matches.emplace_back(
        makeRecording(
            "1",
            "house-a",
            "Movies/Forrest Gump",
            "/srv/vdr/video/Movies/Forrest_Gump.rec"),
        makePerson(
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Forrest Gump"));

    matches.emplace_back(
        makeRecording(
            "2",
            "house-b",
            "Movies/Saving Private Ryan",
            "/srv/vdr/video/Movies/Saving_Private_Ryan.rec"),
        makePerson(
            PersonRole::Director,
            "Steven Spielberg",
            "steven-spielberg",
            ""));

    std::string json =
        serializer.serialize(
            RecordingPersonSearchResult::from(
                matches,
                7,
                2,
                4));

    assert(json.find("\"totalCount\":7") != std::string::npos);
    assert(json.find("\"returnedCount\":2") != std::string::npos);
    assert(json.find("\"limit\":2") != std::string::npos);
    assert(json.find("\"offset\":4") != std::string::npos);
    assert(json.find("\"matches\":[") != std::string::npos);

    assert(json.find("\"recording\":{") != std::string::npos);
    assert(json.find("\"id\":\"1\"") != std::string::npos);
    assert(json.find("\"backendId\":\"house-a\"") != std::string::npos);
    assert(json.find("\"title\":\"Movies/Forrest Gump\"") != std::string::npos);
    assert(json.find("\"path\":\"/srv/vdr/video/Movies/Forrest_Gump.rec\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"/srv/vdr/video/Movies/Forrest_Gump.rec\"") != std::string::npos);
    assert(json.find("\"startTime\":\"1780000000\"") != std::string::npos);
    assert(json.find("\"durationSeconds\":8000") != std::string::npos);
    assert(json.find("\"sizeMb\":7000") != std::string::npos);
    assert(json.find("\"metadata\":{") != std::string::npos);
    assert(json.find("\"presentation\":{") != std::string::npos);
    assert(json.find("\"title\":\"Forrest Gump\"") != std::string::npos);
    assert(json.find("\"summary\":\"Eine vorhandene Aufnahme mit vollständigen Metadaten.\"") != std::string::npos);
    assert(json.find("\"posterUrl\":\"/recording-artwork/house-a/") != std::string::npos);

    assert(json.find("\"person\":{") != std::string::npos);
    assert(json.find("\"source\":\"tvscraper\"") != std::string::npos);
    assert(json.find("\"role\":\"actor\"") != std::string::npos);
    assert(json.find("\"originalName\":\"Tom Hanks\"") != std::string::npos);
    assert(json.find("\"normalizedName\":\"tom-hanks\"") != std::string::npos);
    assert(json.find("\"characterName\":\"Forrest Gump\"") != std::string::npos);
    assert(json.find("\"confidence\":95") != std::string::npos);
    assert(json.find("\"providerReference\":\"tvscraper:example\"") != std::string::npos);

    assert(json.find("\"backendId\":\"house-b\"") != std::string::npos);
    assert(json.find("\"role\":\"director\"") != std::string::npos);
    assert(json.find("\"originalName\":\"Steven Spielberg\"") != std::string::npos);

    matches.clear();
    matches.emplace_back(
        makeRecording(
            "3",
            "house-a",
            "Movies/Quote Test",
            "/srv/vdr/video/Movies/Quote_Test.rec"),
        makePerson(
            PersonRole::Actor,
            "Thomas \"Tom\" Hanks",
            "tom-hanks",
            "Quote Test"));

    std::string escapedJson =
        serializer.serialize(
            RecordingPersonSearchResult::from(
                matches,
                1,
                10,
                0));

    assert(escapedJson.find("Thomas \\\"Tom\\\" Hanks") != std::string::npos);

    std::cout << "test_recording_person_search_result_json_serializer passed" << std::endl;
    return 0;
}

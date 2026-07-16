#include "RestfulApiRecordingMetadataEnricher.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    VdrRecording first;
    first.id = "7";
    first.title = "Technical movie title";

    VdrRecording second;
    second.id = "8";
    second.title = "Technical series title";

    std::vector<VdrRecording> recordings { first, second };

    const std::string json =
        "{\"recordings\":["
        "{\"number\":8,\"event_title\":\"Native series\","
        "\"additional_media\":{\"type\":\"series\","
        "\"series_id\":100,\"episode_id\":208,"
        "\"name\":\"Example Show\","
        "\"episode_season\":2,\"episode_number\":8,"
        "\"episode_name\":\"Poster Day\","
        "\"episode_image\":\"series/100/208.jpg\"}},"
        "{\"number\":7,\"event_title\":\"Native movie\","
        "\"additional_media\":{\"type\":\"movie\","
        "\"movie_id\":13,\"title\":\"Example Movie\","
        "\"poster\":\"movies/13/poster.jpg\"}},"
        "{\"number\":99,\"additional_media\":{"
        "\"type\":\"movie\",\"movie_id\":99}}"
        "]}";

    RestfulApiRecordingMetadataEnricher::enrich(json, recordings);

    assert(recordings.size() == 2);

    assert(recordings[0].metadata.native.eventTitle == "Native movie");
    assert(recordings[0].metadata.provider.contentKind ==
        VdrRecordingContentKind::Movie);
    assert(recordings[0].metadata.provider.movieId == "13");
    assert(recordings[0].metadata.provider.title == "Example Movie");
    assert(recordings[0].metadata.artwork.size() == 1);

    assert(recordings[1].metadata.native.eventTitle == "Native series");
    assert(recordings[1].metadata.provider.contentKind ==
        VdrRecordingContentKind::SeriesEpisode);
    assert(recordings[1].metadata.provider.seriesId == "100");
    assert(recordings[1].metadata.provider.episodeId == "208");
    assert(recordings[1].metadata.provider.seasonNumber == 2);
    assert(recordings[1].metadata.provider.episodeNumber == 8);
    assert(recordings[1].metadata.provider.episodeTitle == "Poster Day");
    assert(recordings[1].metadata.artwork.size() == 1);

    std::cout
        << "test_restful_api_recording_metadata_enricher passed"
        << std::endl;
    return 0;
}

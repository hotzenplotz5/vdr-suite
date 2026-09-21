#include "RestfulApiRecordingMetadataEnricher.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{
void testDirectRecordingMetadataEnrichment()
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
        "\"poster\":\"movies/13/poster.jpg\","
        "\"actors\":[{\"name\":\"Tom Hanks\","
        "\"role\":\"Forrest Gump\"}]}},"
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
    assert(recordings[0].persons.size() == 1);
    assert(recordings[0].persons.hasNormalizedName("tom-hanks"));

    assert(recordings[1].metadata.native.eventTitle == "Native series");
    assert(recordings[1].metadata.provider.contentKind ==
        VdrRecordingContentKind::SeriesEpisode);
    assert(recordings[1].metadata.provider.seriesId == "100");
    assert(recordings[1].metadata.provider.episodeId == "208");
    assert(recordings[1].metadata.provider.seasonNumber == 2);
    assert(recordings[1].metadata.provider.episodeNumber == 8);
    assert(recordings[1].metadata.provider.episodeTitle == "Poster Day");
    assert(recordings[1].metadata.artwork.size() == 1);
}

void testDiscardedAliasEnrichesCanonicalRecording()
{
    VdrRecording canonical;
    canonical.id = "7999";
    canonical.title = "Forrest Gump";
    canonical.path =
        "/VDR-SUITE-TEST_Forrest_Gump/2026-07-08.21.45.2-0.rec";
    canonical.backendNativeId =
        "/srv/vdr/video/VDR-SUITE-TEST_Forrest_Gump/2026-07-08.21.45.2-0.rec";
    canonical.startTime = "1783539900";
    canonical.durationSeconds = 341;
    canonical.sizeMb = 284;

    std::vector<VdrRecording> recordings { canonical };

    const std::string json =
        "{\"recordings\":["
        "{\"number\":3652,"
        "\"name\":\"Recordings on yavdr(nfs)~Forrest Gump\","
        "\"file_name\":\"/srv/vdr/video/Recordings_on_yavdr(nfs)/Forrest_Gump/2026-07-08.21.45.2-0.rec\","
        "\"relative_file_name\":\"/Recordings_on_yavdr(nfs)/Forrest_Gump/2026-07-08.21.45.2-0.rec\","
        "\"duration\":341,\"filesize_mb\":284,"
        "\"event_start_time\":1783539900,"
        "\"additional_media\":{\"type\":\"movie\","
        "\"movie_id\":13,\"title\":\"Forrest Gump\","
        "\"overview\":\"Alias metadata\","
        "\"poster\":\"movies/13/poster.jpg\","
        "\"actors\":["
        "{\"name\":\"Tom Hanks\",\"role\":\"Forrest Gump\"},"
        "{\"name\":\"Robin Wright\",\"role\":\"Jenny Curran\"}"
        "]}},"
        "{\"number\":7999,"
        "\"name\":\"VDR-SUITE-TEST Forrest Gump\","
        "\"file_name\":\"/srv/vdr/video/VDR-SUITE-TEST_Forrest_Gump/2026-07-08.21.45.2-0.rec\","
        "\"relative_file_name\":\"/VDR-SUITE-TEST_Forrest_Gump/2026-07-08.21.45.2-0.rec\","
        "\"duration\":341,\"filesize_mb\":284,"
        "\"event_start_time\":1783539900}"
        "]}";

    RestfulApiRecordingMetadataEnricher::enrich(json, recordings);

    assert(recordings.size() == 1);
    assert(recordings[0].id == "7999");
    assert(recordings[0].metadata.provider.movieId == "13");
    assert(recordings[0].metadata.provider.title == "Forrest Gump");
    assert(recordings[0].metadata.provider.overview == "Alias metadata");
    assert(recordings[0].metadata.artwork.size() == 1);
    assert(recordings[0].persons.size() == 2);
    assert(recordings[0].persons.hasNormalizedName("tom-hanks"));
    assert(recordings[0].persons.hasNormalizedName("robin-wright"));
}
}

int main()
{
    testDirectRecordingMetadataEnrichment();
    testDiscardedAliasEnrichesCanonicalRecording();

    std::cout
        << "test_restful_api_recording_metadata_enricher passed"
        << std::endl;
    return 0;
}

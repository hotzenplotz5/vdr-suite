#include "RestfulApiRecordingMetadataMapper.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

namespace
{

const VdrRecordingArtworkRef* findArtwork(
    const VdrRecordingMetadata& metadata,
    const VdrRecordingArtworkKind kind)
{
    for (const VdrRecordingArtworkRef& artwork : metadata.artwork)
    {
        if (artwork.kind == kind)
        {
            return &artwork;
        }
    }

    return nullptr;
}

void test_maps_native_event_text_without_provider_data()
{
    const VdrRecordingMetadata metadata =
        RestfulApiRecordingMetadataMapper::mapRecordingObject(
            "{"
            "\"event_title\":\"Tatort\","
            "\"event_short_text\":\"Borowski ermittelt\","
            "\"event_description\":\"Ein langer Beschreibungstext.\""
            "}");

    assert(metadata.native.eventTitle == "Tatort");
    assert(metadata.native.shortText == "Borowski ermittelt");
    assert(metadata.native.description ==
        "Ein langer Beschreibungstext.");
    assert(metadata.native.hasText());
    assert(!metadata.hasProviderData());
    assert(!metadata.hasArtwork());
}

void test_maps_current_movie_shape_and_temporary_artwork_refs()
{
    const std::string json =
        "{"
        "\"event_title\":\"Forrest Gump\","
        "\"event_short_text\":\"Drama\","
        "\"event_description\":\"Native VDR description\","
        "\"additional_media\":{"
        "\"type\":\"movie\","
        "\"movie_id\":13,"
        "\"title\":\"Forrest Gump\","
        "\"original_title\":\"Forrest Gump\","
        "\"tagline\":\"Life is like a box of chocolates\","
        "\"overview\":\"A life story.\","
        "\"genres\":\"Drama, Comedy\","
        "\"release_date\":\"1994-07-06\","
        "\"runtime\":142,"
        "\"vote_average\":8.48,"
        "\"poster\":\"movies/13/poster.jpg\","
        "\"fanart\":\"movies/13/fanart.jpg\""
        "}"
        "}";

    const VdrRecordingMetadata metadata =
        RestfulApiRecordingMetadataMapper::mapRecordingObject(json);

    assert(metadata.hasProviderData());
    assert(metadata.provider.source ==
        VdrRecordingMetadataSource::RestfulApiScraperBridge);
    assert(metadata.provider.contentKind ==
        VdrRecordingContentKind::Movie);
    assert(metadata.provider.movieId == "13");
    assert(metadata.provider.title == "Forrest Gump");
    assert(metadata.provider.originalTitle == "Forrest Gump");
    assert(metadata.provider.tagline ==
        "Life is like a box of chocolates");
    assert(metadata.provider.overview == "A life story.");
    assert(metadata.provider.genreText == "Drama, Comedy");
    assert(metadata.provider.releaseDate == "1994-07-06");
    assert(metadata.provider.runtimeMinutes == 142);
    assert(std::fabs(metadata.provider.rating - 8.48) < 0.001);
    assert(metadata.hasArtwork());
    assert(metadata.artwork.size() == 2);

    const VdrRecordingArtworkRef* poster =
        findArtwork(metadata, VdrRecordingArtworkKind::Poster);
    const VdrRecordingArtworkRef* fanart =
        findArtwork(metadata, VdrRecordingArtworkKind::Fanart);

    assert(poster != nullptr);
    assert(poster->reference == "movies/13/poster.jpg");
    assert(poster->temporary);
    assert(poster->source ==
        VdrRecordingMetadataSource::RestfulApiScraperBridge);
    assert(fanart != nullptr);
    assert(fanart->reference == "movies/13/fanart.jpg");
}

void test_maps_current_series_shape_and_image_dimensions()
{
    const std::string json =
        "{"
        "\"additional_media\":{"
        "\"type\":\"series\","
        "\"series_id\":100,"
        "\"episode_id\":208,"
        "\"name\":\"The Example Show\","
        "\"overview\":\"Series overview\","
        "\"genre\":\"Drama\","
        "\"rating\":7.1,"
        "\"episode_number\":8,"
        "\"episode_season\":2,"
        "\"episode_name\":\"The Poster Episode\","
        "\"episode_first_aired\":\"2026-07-16\","
        "\"episode_overview\":\"Episode overview\","
        "\"episode_rating\":8.7,"
        "\"episode_image\":\"series/100/episodes/208.jpg\","
        "\"posters\":["
        "{\"path\":\"series/100/poster-a.jpg\",\"width\":680,\"height\":1000},"
        "{\"path\":\"series/100/poster-b.jpg\",\"width\":340,\"height\":500}"
        "],"
        "\"banners\":["
        "{\"path\":\"series/100/banner.jpg\",\"width\":758,\"height\":140}"
        "],"
        "\"fanarts\":["
        "{\"path\":\"series/100/fanart.jpg\",\"width\":1920,\"height\":1080}"
        "]"
        "}"
        "}";

    const VdrRecordingMetadata metadata =
        RestfulApiRecordingMetadataMapper::mapRecordingObject(json);

    assert(metadata.hasProviderData());
    assert(metadata.provider.contentKind ==
        VdrRecordingContentKind::SeriesEpisode);
    assert(metadata.provider.seriesId == "100");
    assert(metadata.provider.episodeId == "208");
    assert(metadata.provider.seriesTitle == "The Example Show");
    assert(metadata.provider.episodeTitle == "The Poster Episode");
    assert(metadata.provider.title == "The Poster Episode");
    assert(metadata.provider.overview == "Episode overview");
    assert(metadata.provider.genreText == "Drama");
    assert(metadata.provider.releaseDate == "2026-07-16");
    assert(metadata.provider.seasonNumber == 2);
    assert(metadata.provider.episodeNumber == 8);
    assert(std::fabs(metadata.provider.rating - 8.7) < 0.001);
    assert(metadata.artwork.size() == 5);

    const VdrRecordingArtworkRef* poster =
        findArtwork(metadata, VdrRecordingArtworkKind::Poster);
    const VdrRecordingArtworkRef* still =
        findArtwork(metadata, VdrRecordingArtworkKind::Still);
    const VdrRecordingArtworkRef* banner =
        findArtwork(metadata, VdrRecordingArtworkKind::Banner);
    const VdrRecordingArtworkRef* fanart =
        findArtwork(metadata, VdrRecordingArtworkKind::Fanart);

    assert(poster != nullptr);
    assert(poster->reference == "series/100/poster-a.jpg");
    assert(poster->width == 680);
    assert(poster->height == 1000);
    assert(still != nullptr);
    assert(still->reference == "series/100/episodes/208.jpg");
    assert(banner != nullptr);
    assert(fanart != nullptr);
}

void test_preserves_legacy_field_aliases_during_transition()
{
    const VdrRecordingMetadata metadata =
        RestfulApiRecordingMetadataMapper::mapRecordingObject(
            "{\"additional_media\":{"
            "\"scraper\":\"movie\","
            "\"movie_id\":7,"
            "\"movie_title\":\"Legacy Movie\","
            "\"movie_overview\":\"Legacy overview\","
            "\"movie_poster\":\"legacy/poster.jpg\""
            "}}");

    assert(metadata.hasProviderData());
    assert(metadata.provider.contentKind ==
        VdrRecordingContentKind::Movie);
    assert(metadata.provider.movieId == "7");
    assert(metadata.provider.title == "Legacy Movie");
    assert(metadata.provider.overview == "Legacy overview");
    assert(metadata.artwork.size() == 1);
    assert(metadata.artwork.front().reference ==
        "legacy/poster.jpg");
}

void test_rejects_urls_absolute_paths_and_traversal_refs()
{
    const VdrRecordingMetadata metadata =
        RestfulApiRecordingMetadataMapper::mapRecordingObject(
            "{\"additional_media\":{"
            "\"type\":\"series\","
            "\"series_id\":9,"
            "\"episode_image\":\"https://example.invalid/image.jpg\","
            "\"posters\":["
            "{\"path\":\"/etc/passwd\",\"width\":1,\"height\":1},"
            "{\"path\":\"series/../secret.jpg\",\"width\":1,\"height\":1},"
            "{\"path\":\"series/%2e%2e/encoded.jpg\",\"width\":1,\"height\":1}"
            "]"
            "}}");

    assert(metadata.hasProviderData());
    assert(!metadata.hasArtwork());
    assert(metadata.artwork.empty());
}

void test_unknown_additional_media_does_not_claim_provider_state()
{
    const VdrRecordingMetadata metadata =
        RestfulApiRecordingMetadataMapper::mapRecordingObject(
            "{\"additional_media\":{"
            "\"type\":\"unsupported\","
            "\"actors\":[{\"name\":\"Example\"}]"
            "}}");

    assert(!metadata.hasProviderData());
    assert(metadata.provider.contentKind ==
        VdrRecordingContentKind::Unknown);
    assert(!metadata.hasArtwork());
}

}

int main()
{
    test_maps_native_event_text_without_provider_data();
    test_maps_current_movie_shape_and_temporary_artwork_refs();
    test_maps_current_series_shape_and_image_dimensions();
    test_preserves_legacy_field_aliases_during_transition();
    test_rejects_urls_absolute_paths_and_traversal_refs();
    test_unknown_additional_media_does_not_claim_provider_state();

    std::cout
        << "test_restful_api_recording_metadata_mapper passed"
        << std::endl;

    return 0;
}

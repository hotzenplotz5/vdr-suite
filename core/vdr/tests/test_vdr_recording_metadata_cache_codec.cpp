#include "VdrRecordingMetadataCacheCodec.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    VdrRecordingMetadata metadata;
    metadata.native.eventTitle = "Native title";
    metadata.native.shortText = "Short: text";
    metadata.native.description = "Description with ä and : delimiters";
    metadata.provider.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    metadata.provider.contentKind =
        VdrRecordingContentKind::SeriesEpisode;
    metadata.provider.seriesId = "100";
    metadata.provider.episodeId = "208";
    metadata.provider.seriesTitle = "Example Show";
    metadata.provider.episodeTitle = "Poster Day";
    metadata.provider.overview = "Episode overview";
    metadata.provider.seasonNumber = 2;
    metadata.provider.episodeNumber = 8;
    metadata.provider.runtimeMinutes = 47;
    metadata.provider.rating = 8.75;

    VdrRecordingArtworkRef poster;
    poster.kind = VdrRecordingArtworkKind::Poster;
    poster.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    poster.reference = "series/100/poster.jpg";
    poster.width = 680;
    poster.height = 1000;
    poster.temporary = true;
    metadata.artwork.push_back(poster);

    VdrRecordingArtworkRef still;
    still.kind = VdrRecordingArtworkKind::Still;
    still.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    still.reference = "series/100/episodes/208.jpg";
    still.width = 1280;
    still.height = 720;
    still.temporary = true;
    metadata.artwork.push_back(still);

    const std::string payload =
        VdrRecordingMetadataCacheCodec::encode(metadata);

    assert(payload.rfind("VRM1", 0) == 0);

    const VdrRecordingMetadata decoded =
        VdrRecordingMetadataCacheCodec::decode(payload);

    assert(decoded.native.eventTitle == "Native title");
    assert(decoded.native.shortText == "Short: text");
    assert(decoded.native.description ==
        "Description with ä and : delimiters");
    assert(decoded.provider.source ==
        VdrRecordingMetadataSource::RestfulApiScraperBridge);
    assert(decoded.provider.contentKind ==
        VdrRecordingContentKind::SeriesEpisode);
    assert(decoded.provider.seriesId == "100");
    assert(decoded.provider.episodeId == "208");
    assert(decoded.provider.seriesTitle == "Example Show");
    assert(decoded.provider.episodeTitle == "Poster Day");
    assert(decoded.provider.seasonNumber == 2);
    assert(decoded.provider.episodeNumber == 8);
    assert(decoded.provider.runtimeMinutes == 47);
    assert(std::fabs(decoded.provider.rating - 8.75) < 0.001);
    assert(decoded.artwork.size() == 2);
    assert(decoded.artwork[0].reference == "series/100/poster.jpg");
    assert(decoded.artwork[0].width == 680);
    assert(decoded.artwork[1].kind == VdrRecordingArtworkKind::Still);

    assert(VdrRecordingMetadataCacheCodec::encode(
        VdrRecordingMetadata{}).empty());

    assert(!VdrRecordingMetadataCacheCodec::decode(
        "broken").hasProviderData());
    assert(!VdrRecordingMetadataCacheCodec::decode(
        payload.substr(0, payload.size() - 1)).hasProviderData());
    assert(!VdrRecordingMetadataCacheCodec::decode(
        payload + "unexpected").hasProviderData());

    std::cout
        << "test_vdr_recording_metadata_cache_codec passed"
        << std::endl;
    return 0;
}

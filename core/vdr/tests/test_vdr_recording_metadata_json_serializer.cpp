#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingMetadataJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    VdrRecording recording;
    recording.id = "7";
    recording.backendId = "ferienhaus";
    recording.title = "Technical title";
    recording.metadata.native.eventTitle = "Native event title";
    recording.metadata.native.shortText = "Native short text";
    recording.metadata.native.description = "Native description";
    recording.metadata.provider.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    recording.metadata.provider.contentKind =
        VdrRecordingContentKind::SeriesEpisode;
    recording.metadata.provider.seriesId = "100";
    recording.metadata.provider.episodeId = "208";
    recording.metadata.provider.seriesTitle = "Example Show";
    recording.metadata.provider.episodeTitle = "Poster Day";
    recording.metadata.provider.overview = "Provider overview";
    recording.metadata.provider.seasonNumber = 2;
    recording.metadata.provider.episodeNumber = 8;
    recording.metadata.provider.rating = 8.5;

    VdrRecordingArtworkRef artwork;
    artwork.kind = VdrRecordingArtworkKind::Poster;
    artwork.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    artwork.reference = "series/100/poster-secret.jpg";
    artwork.width = 680;
    artwork.height = 1000;
    recording.metadata.artwork.push_back(artwork);

    const std::string expectedAssetId =
        VdrRecordingArtworkIdentity::assetId(
            recording,
            recording.metadata.artwork.front());
    const std::string expectedUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            recording,
            recording.metadata.artwork.front());

    const std::string json =
        VdrRecordingMetadataJsonSerializer::serialize(recording);

    assert(json.find("\"available\":true") != std::string::npos);
    assert(json.find("\"source\":\"restfulapi-scraper-bridge\"") !=
        std::string::npos);
    assert(json.find("\"contentKind\":\"series-episode\"") !=
        std::string::npos);
    assert(json.find("\"seriesTitle\":\"Example Show\"") !=
        std::string::npos);
    assert(json.find("\"episodeTitle\":\"Poster Day\"") !=
        std::string::npos);
    assert(json.find("\"title\":\"Example Show\"") !=
        std::string::npos);
    assert(json.find("\"subtitle\":\"S02E08 · Poster Day\"") !=
        std::string::npos);
    assert(json.find("\"summary\":\"Provider overview\"") !=
        std::string::npos);
    assert(json.find("\"posterAvailable\":true") !=
        std::string::npos);
    assert(json.find("\"artworkPrepared\":true") !=
        std::string::npos);
    assert(json.find("\"placeholderVariant\":") !=
        std::string::npos);
    assert(json.find(
        "\"preferredAssetId\":\"" + expectedAssetId + "\"") !=
        std::string::npos);
    assert(json.find(
        "\"posterAssetId\":\"" + expectedAssetId + "\"") !=
        std::string::npos);
    assert(json.find(
        "\"preferredUrl\":\"" + expectedUrl + "\"") !=
        std::string::npos);
    assert(json.find(
        "\"posterUrl\":\"" + expectedUrl + "\"") !=
        std::string::npos);
    assert(expectedUrl.find("/recording-artwork/ferienhaus/") == 0);

    // Source-scoped references are internal cache evidence. The client JSON
    // exposes an opaque Suite URL and never leaks provider paths.
    assert(json.find("poster-secret.jpg") == std::string::npos);
    assert(json.find("series/100") == std::string::npos);

    VdrRecording nativeFallback;
    nativeFallback.backendId = "living room";
    nativeFallback.backendNativeId =
        "/srv/vdr/video/Serien/Band of Brothers/01 Currahee/2016.rec";
    nativeFallback.title = "01 Currahee";

    const std::string nativeFallbackUrl =
        "/api/vdr/recordings/metadata/image?backend=living%20room"
        "&backendNativeId=%2Fsrv%2Fvdr%2Fvideo%2FSerien%2FBand%20of%20Brothers%2F01%20Currahee%2F2016.rec"
        "&kind=preferred&index=0";
    const std::string nativeFallbackJson =
        VdrRecordingMetadataJsonSerializer::serialize(nativeFallback);

    assert(nativeFallbackJson.find("\"artworkPrepared\":false") !=
        std::string::npos);
    assert(nativeFallbackJson.find("\"posterAvailable\":false") !=
        std::string::npos);
    assert(nativeFallbackJson.find(
        "\"preferredUrl\":\"" + nativeFallbackUrl + "\"") !=
        std::string::npos);
    assert(nativeFallbackJson.find(
        "\"posterUrl\":\"" + nativeFallbackUrl + "\"") !=
        std::string::npos);

    VdrRecording fallback;
    fallback.title = "Fallback Recording";

    const std::string fallbackJson =
        VdrRecordingMetadataJsonSerializer::serialize(fallback);

    assert(fallbackJson.find("\"providerAvailable\":false") !=
        std::string::npos);
    assert(fallbackJson.find("\"artworkPrepared\":false") !=
        std::string::npos);
    assert(fallbackJson.find("\"posterAssetId\":\"\"") !=
        std::string::npos);
    assert(fallbackJson.find("\"posterUrl\":\"\"") !=
        std::string::npos);
    assert(fallbackJson.find("\"title\":\"Fallback Recording\"") !=
        std::string::npos);

    std::cout
        << "test_vdr_recording_metadata_json_serializer passed"
        << std::endl;
    return 0;
}

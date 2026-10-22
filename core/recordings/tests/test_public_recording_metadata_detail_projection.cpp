#include "PublicRecordingMetadataDetailProjection.h"

#include <cassert>
#include <string>

namespace
{
PublicRecordingReadItem publicItem()
{
    PublicRecordingReadItem value;
    value.recordingId = "rec_0123456789abcdef0123456789abcdef";
    value.backendId = "backend-a";
    value.title = "VDR title";
    value.recordedAt = "2026-10-20T20:15:00Z";
    value.durationSeconds = 3600;
    value.durationKnown = true;
    value.description = "Synopsis";
    return value;
}

VdrRecording source()
{
    VdrRecording recording;
    recording.backendId = "backend-a";
    recording.backendNativeId = "/private/recording/2026.rec";
    recording.id = "untrusted-native-id";
    recording.path = "/private/video/2026.rec";
    recording.title = "VDR title";
    return recording;
}
}

int main()
{
    auto original = source();
    auto publicRecording = publicItem();
    auto result = PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording);
    assert(result.has_value());
    assert(result->metadataState == "absent");
    assert(result->metadataSource == "none");
    assert(result->contentKind == "unknown");
    assert(result->recording.recordingId == publicRecording.recordingId);
    assert(result->recording.backendId == "backend-a");

    original.metadata.native.eventTitle = "Native Event";
    original.metadata.native.shortText = "Native subtitle";
    original.metadata.native.description = "Native synopsis";
    result = PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording);
    assert(result && result->metadataState == "partial");
    assert(result->subtitle == "Native subtitle");
    assert(result->epgTitle == "Native Event");
    assert(result->epgDescription == "Native synopsis");

    auto& provider = original.metadata.provider;
    provider.source = VdrRecordingMetadataSource::RestfulApiScraperBridge;
    provider.contentKind = VdrRecordingContentKind::SeriesEpisode;
    provider.seriesTitle = "Series";
    provider.episodeTitle = "Episode";
    provider.originalTitle = "Original";
    provider.genreText = "Documentary";
    provider.releaseDate = "2026-10-20";
    provider.seasonNumber = 3;
    provider.episodeNumber = 7;
    provider.runtimeMinutes = 52;
    provider.movieId = "private-provider-movie-id";
    provider.seriesId = "private-provider-series-id";
    provider.episodeId = "private-provider-episode-id";
    original.metadata.artwork.push_back({
        VdrRecordingArtworkKind::Poster,
        VdrRecordingMetadataSource::RestfulApiScraperBridge,
        "/srv/private/poster.jpg", 720, 1080, false
    });
    result = PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording);
    assert(result && result->metadataState == "available");
    assert(result->metadataSource == "restfulapi-scraper-bridge");
    assert(result->contentKind == "series-episode");
    assert(result->seasonNumber == 3 && result->episodeNumber == 7);
    assert(result->providerRuntimeMinutes == 52);
    assert(result->genreText == "Documentary");
    assert(result->originalTitle == "Original");

    // Unknown / incomplete provider claims must not fabricate details.
    provider.title.clear();
    provider.originalTitle.clear();
    provider.overview.clear();
    provider.seriesTitle.clear();
    provider.episodeTitle.clear();
    provider.genreText.clear();
    assert(PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original,
        publicRecording)->metadataState == "partial");

    // UTF-8-safe bounded text, including a truncated multibyte boundary.
    original.metadata.native.shortText =
        std::string(1023, 'x') + "\xc3\xa4" + "tail";
    result = PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording);
    assert(result && result->subtitle == std::string(1023, 'x'));

    // Source provenance, public identity and exact backend scope are strict.
    assert(!PublicRecordingMetadataDetailProjection::project(
        "backend-b", publicRecording.recordingId, original, publicRecording));
    assert(!PublicRecordingMetadataDetailProjection::project(
        "*", publicRecording.recordingId, original, publicRecording));
    assert(!PublicRecordingMetadataDetailProjection::project(
        "backend-a", "rec_ffffffffffffffffffffffffffffffff",
        original, publicRecording));
    publicRecording.recordingId = "rec_0123456789abcdef0123456789abcdef";
    original.backendNativeId.clear();
    assert(!PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording));

    return 0;
}

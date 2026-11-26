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

    // Only transmit native description separately if it adds information
    // beyond the existing collection's preferred description.
    publicRecording.description = "Native synopsis";
    result = PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording);
    assert(result && result->epgDescription.empty());
    publicRecording.description = "Synopsis";

    auto& provider = original.metadata.provider;
    provider.source = VdrRecordingMetadataSource::RestfulApiScraperBridge;
    provider.title = "Explicit provider title";
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
    assert(result->providerTitle == "Explicit provider title");

    VdrRecordingNativeMetadataRecord native;
    native.backendId = original.backendId;
    native.backendNativeId = original.backendNativeId;
    native.recordingKey = "private-recording-key";
    native.metadata.found = true;
    native.metadata.voteAverage = 7.5;
    native.metadata.voteCount = 123;
    native.metadata.firstAired = "2026-10-19";
    native.metadata.status = "Returning Series";
    native.metadata.collectionName = "Collection";
    native.metadata.absoluteEpisodeNumber = 24;
    native.metadata.genres = {"Drama", "Mystery"};
    native.metadata.productionCountries = {"DE"};
    native.metadata.networks = {"Network"};
    VdrRecordingNativePerson cast;
    cast.name = "Example Actor";
    cast.role = "actor";
    cast.characterName = "Example Character";
    cast.image.path = "/private/actor.jpg";
    native.metadata.people.push_back(cast);
    PublicRecordingMetadataDetailProjection::applyNativeDetails(original, native, *result);
    assert(result->voteAverage == 7.5 && result->voteCount == 123);
    assert(result->firstAired == "2026-10-19");
    assert(result->genres.size() == 2 && result->networks.size() == 1);
    assert(result->people.size() == 1 && result->people.front().name == "Example Actor");
    auto wrongNative = native;
    wrongNative.backendId = "backend-b";
    wrongNative.metadata.people.front().name = "Other Backend";
    PublicRecordingMetadataDetailProjection::applyNativeDetails(original, wrongNative, *result);
    assert(result->people.front().name == "Example Actor");

    // Unknown / incomplete provider claims must not fabricate details.
    provider.title.clear();
    provider.originalTitle.clear();
    provider.overview.clear();
    provider.seriesTitle.clear();
    provider.episodeTitle.clear();
    provider.genreText.clear();
    provider.releaseDate.clear();
    provider.seasonNumber = 0;
    provider.episodeNumber = 0;
    provider.runtimeMinutes = 0;
    assert(PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original,
        publicRecording)->metadataState == "partial");

    // A legacy source claiming no provider must not leak orphaned values.
    provider.source = VdrRecordingMetadataSource::None;
    provider.contentKind = VdrRecordingContentKind::Movie;
    provider.title = "Untrusted orphan title";
    provider.originalTitle = "Untrusted orphan original";
    provider.seasonNumber = 9;
    result = PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording);
    assert(result && result->metadataSource == "none");
    assert(result->contentKind == "unknown");
    assert(result->metadataState == "partial");
    assert(result->providerTitle.empty());
    assert(result->originalTitle.empty());
    assert(result->seasonNumber == 0);
    provider.source = VdrRecordingMetadataSource::RestfulApiScraperBridge;

    // UTF-8-safe bounded text, including a truncated multibyte boundary.
    original.metadata.native.shortText =
        std::string(1023, 'x') + "\xc3\xa4" + "tail";
    result = PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording);
    assert(result && result->subtitle == std::string(1023, 'x'));

    // M1.2 existing manual assignment wins over scraper metadata, but
    // native EPG and public Recording title/identity stay unchanged.
    ManualRecordingMetadataAssignment manual;
    manual.found = true;
    manual.backendId = "backend-a";
    manual.resourceKey = "/private/source/key/not-public";
    manual.metadataAssignmentId = "mdasg_" + std::string(32, 'a');
    manual.metadataEntityId = "mdent_" + std::string(32, 'b');
    manual.metadataTargetId = "mdtgt_" + std::string(32, 'c');
    manual.relationshipLocked = true;
    manual.revision = 4;
    manual.mediaType = "episode";
    manual.title = "Manual Episode";
    manual.originalTitle = "Manuell Original";
    manual.overview = "Manual synopsis";
    manual.releaseDate = "2025-03-09";
    manual.seasonNumber = 2;
    manual.episodeNumber = 5;
    ManualRecordingMetadataPerson selectedPerson;
    selectedPerson.name = "Selected Actor";
    selectedPerson.characterName = "Selected Character";
    selectedPerson.profilePath = "/private/manual-actor.jpg";
    manual.people.push_back(selectedPerson);
    manual.posterReference = "/srv/private/untrusted-manual-poster.png";
    manual.providerId = "secret-provider";
    manual.externalId = "secret-remote-id";
    auto manualDetail = PublicRecordingMetadataDetailProjection::project(
        "backend-a", publicRecording.recordingId, original, publicRecording);
    assert(manualDetail.has_value());
    PublicRecordingMetadataDetailProjection::applyNativeDetails(original, native, *manualDetail);
    assert(manualDetail->people.size() == 1);
    assert(PublicRecordingMetadataDetailProjection::applyManualAssignment(
        "backend-a", manual, *manualDetail));
    assert(manualDetail->metadataSource == "manual");
    assert(manualDetail->metadataState == "available");
    assert(manualDetail->contentKind == "series-episode");
    assert(manualDetail->providerTitle == "Manual Episode");
    assert(manualDetail->originalTitle == "Manuell Original");
    assert(manualDetail->overview == "Manual synopsis");
    assert(manualDetail->seasonNumber == 2 && manualDetail->episodeNumber == 5);
    assert(manualDetail->recording.title == "VDR title");
    // Projection keeps native subtitle but applies its 1024-byte UTF-8 bound.
    assert(manualDetail->subtitle == std::string(1023, 'x'));
    assert(manualDetail->manualRevision == 4);
    assert(manualDetail->people.size() == 1);
    assert(manualDetail->people.front().name == "Selected Actor");
    assert(manualDetail->voteAverage == 0.0 && manualDetail->genres.empty());
    PublicRecordingMetadataDetailProjection::applyNativeDetails(original, native, *manualDetail);
    assert(manualDetail->people.front().name == "Selected Actor");
    assert(manualDetail->manualAssignmentId == manual.metadataAssignmentId);
    assert(manualDetail->metadataEntityId == manual.metadataEntityId);
    assert(manualDetail->tagline.empty());
    assert(manualDetail->genreText.empty());
    assert(manualDetail->providerRuntimeMinutes == 0);
    auto wrongManual = manual;
    wrongManual.backendId = "backend-b";
    assert(!PublicRecordingMetadataDetailProjection::applyManualAssignment(
        "backend-a", wrongManual, *manualDetail));
    wrongManual = manual;
    wrongManual.metadataAssignmentId = "mdasg_invalid";
    assert(!PublicRecordingMetadataDetailProjection::applyManualAssignment(
        "backend-a", wrongManual, *manualDetail));
    wrongManual = manual;
    wrongManual.relationshipLocked = false;
    assert(!PublicRecordingMetadataDetailProjection::applyManualAssignment(
        "backend-a", wrongManual, *manualDetail));
    wrongManual = manual;
    wrongManual.revision = 0;
    assert(!PublicRecordingMetadataDetailProjection::applyManualAssignment(
        "backend-a", wrongManual, *manualDetail));
    wrongManual = manual;
    wrongManual.found = false;
    assert(PublicRecordingMetadataDetailProjection::applyManualAssignment(
        "backend-a", wrongManual, *manualDetail));

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

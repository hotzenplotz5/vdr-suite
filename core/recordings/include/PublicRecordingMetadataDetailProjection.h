#pragma once

#include "PublicRecordingCollectionProjection.h"
#include "VdrRecording.h"

#include <algorithm>
#include <optional>
#include <string>

// M1: bounded public detail projection of already resolved, backend-scoped
// Recording metadata. Callers must authenticate/authorize recordings.view and
// resolve rec_ through the existing identity repository and Recording cache.
// This DTO intentionally contains no native IDs, filesystem paths, provider
// artwork references, or mutable metadata.
struct PublicRecordingMetadataDetail
{
    PublicRecordingReadItem recording;
    std::string metadataState; // absent, partial, available
    std::string metadataSource; // native VDR data remains separate
    std::string contentKind;    // unknown, movie, series-episode
    std::string epgTitle;
    std::string subtitle;
    std::string epgDescription;
    std::string originalTitle;
    std::string tagline;
    std::string seriesTitle;
    std::string episodeTitle;
    std::string genreText;
    std::string releaseDate;
    int seasonNumber = 0;
    int episodeNumber = 0;
    int providerRuntimeMinutes = 0;
};

// Caller supplies the public item obtained from the existing collection
// projection, not a caller-provided native VDR identity. Fail closed when
// projection identity or backend provenance does not match the resolved source.
class PublicRecordingMetadataDetailProjection final
{
public:
    static std::optional<PublicRecordingMetadataDetail> project(
        const std::string& authorizedBackendId,
        const std::string& requestedRecordingId,
        const VdrRecording& source,
        const PublicRecordingReadItem& publicItem)
    {
        if (authorizedBackendId.empty() || authorizedBackendId == "*" ||
            source.backendId != authorizedBackendId ||
            source.backendNativeId.empty() ||
            publicItem.backendId != authorizedBackendId ||
            requestedRecordingId != publicItem.recordingId ||
            requestedRecordingId.size() != 36U ||
            requestedRecordingId.compare(0U, 4U, "rec_") != 0 ||
            !std::all_of(requestedRecordingId.begin() + 4,
                requestedRecordingId.end(), [](unsigned char c) {
                    return (c >= '0' && c <= '9') ||
                           (c >= 'a' && c <= 'f');
                }) ||
            publicItem.title.empty() ||
            publicItem.durationSeconds < 0 ||
            publicItem.sizeMb < 0)
            return std::nullopt;

        const auto& native = source.metadata.native;
        const auto& provider = source.metadata.provider;

        PublicRecordingMetadataDetail item;
        item.recording = publicItem;
        item.metadataSource = vdrRecordingMetadataSourceName(provider.source);
        item.contentKind = vdrRecordingContentKindName(provider.contentKind);
        item.epgTitle = bounded(native.eventTitle, 1024U);
        item.subtitle = bounded(native.shortText, 1024U);
        item.epgDescription = bounded(native.description, 4096U);
        item.originalTitle = bounded(provider.originalTitle, 1024U);
        item.tagline = bounded(provider.tagline, 1024U);
        item.seriesTitle = bounded(provider.seriesTitle, 1024U);
        item.episodeTitle = bounded(provider.episodeTitle, 1024U);
        item.genreText = bounded(provider.genreText, 1024U);
        item.releaseDate = bounded(provider.releaseDate, 32U);
        item.seasonNumber = std::max(0, provider.seasonNumber);
        item.episodeNumber = std::max(0, provider.episodeNumber);
        item.providerRuntimeMinutes = std::max(0, provider.runtimeMinutes);

        const bool hasNative = native.hasText();
        const bool hasProvider = provider.hasData();
        const bool hasUsefulProvider =
            !provider.title.empty() || !provider.originalTitle.empty() ||
            !provider.overview.empty() || !provider.seriesTitle.empty() ||
            !provider.episodeTitle.empty() || !provider.genreText.empty();
        item.metadataState = hasProvider && hasUsefulProvider
            ? "available"
            : (hasNative || hasProvider ? "partial" : "absent");

        return item;
    }

private:
    static std::string bounded(const std::string& value, std::size_t maxBytes)
    {
        if (value.size() <= maxBytes) return value;
        std::size_t limit = maxBytes;
        while (limit > 0U &&
            (static_cast<unsigned char>(value[limit]) & 0xc0U) == 0x80U)
            --limit;
        return value.substr(0U, limit);
    }
};

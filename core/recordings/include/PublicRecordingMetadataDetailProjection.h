#pragma once

#include "PublicRecordingCollectionProjection.h"
#include "ManualRecordingMetadataAssignmentRepository.h"
#include "MetadataIdentity.h"
#include "VdrRecording.h"
#include "VdrRecordingNativeMetadataRepository.h"

#include <algorithm>
#include <cmath>
#include <vector>
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
    std::string providerTitle;
    std::string overview;
    std::string originalTitle;
    std::string tagline;
    std::string seriesTitle;
    std::string episodeTitle;
    std::string genreText;
    std::string releaseDate;
    int seasonNumber = 0;
    int episodeNumber = 0;
    int providerRuntimeMinutes = 0;
    std::string manualAssignmentId;
    std::string metadataEntityId;
    int manualRevision = 0;
    // Optional detail facts from the existing persisted Scraper/manual read.
    // No provider identity, local artwork reference or backend path.
    std::string firstAired;
    std::string statusText;
    std::string collectionName;
    std::string imdbId;
    int absoluteEpisodeNumber = 0;
    double voteAverage = 0.0;
    int voteCount = 0;
    std::vector<std::string> genres;
    std::vector<std::string> productionCountries;
    std::vector<std::string> networks;
    struct Person
    {
        std::string role;
        std::string name;
        std::string characterName;
        std::size_t sourceIndex = 0; // internal ordinal for the same cast's image
    };
    std::vector<Person> people;
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
        const bool hasProvider = provider.hasData();
        item.metadataSource = hasProvider
            ? vdrRecordingMetadataSourceName(provider.source) : "none";
        item.contentKind = hasProvider
            ? vdrRecordingContentKindName(provider.contentKind) : "unknown";
        item.epgTitle = bounded(native.eventTitle, 1024U);
        item.subtitle = bounded(native.shortText, 1024U);
        item.epgDescription = bounded(native.description, 4096U);
        // Do not send an identical description twice. If VDR's longer
        // description contains more than the collection's 2048-byte excerpt,
        // keep the additional detail as epgDescription.
        if (item.epgDescription == publicItem.description)
            item.epgDescription.clear();

        // Provider fields are untrusted unless a known source actually claims
        // them. This also prevents source=none from emitting orphaned values.
        if (hasProvider)
        {
            item.providerTitle = bounded(provider.title, 1024U);
            item.overview = bounded(provider.overview, 4096U);
            item.originalTitle = bounded(provider.originalTitle, 1024U);
            item.tagline = bounded(provider.tagline, 1024U);
            item.seriesTitle = bounded(provider.seriesTitle, 1024U);
            item.episodeTitle = bounded(provider.episodeTitle, 1024U);
            item.genreText = bounded(provider.genreText, 1024U);
            item.releaseDate = bounded(provider.releaseDate, 32U);
            item.seasonNumber = std::max(0, provider.seasonNumber);
            item.episodeNumber = std::max(0, provider.episodeNumber);
            item.providerRuntimeMinutes = std::max(0, provider.runtimeMinutes);
        }

        if (item.overview == publicItem.description)
            item.overview.clear();

        if (hasProvider && std::isfinite(provider.rating) &&
            provider.rating > 0.0 && provider.rating <= 10.0)
            item.voteAverage = provider.rating;
        const bool hasNative = native.hasText();
        const bool hasUsefulProvider = hasProvider &&
            (!item.providerTitle.empty() || !item.originalTitle.empty() ||
             !provider.overview.empty() || !item.seriesTitle.empty() ||
             !item.episodeTitle.empty() || !item.genreText.empty() ||
             !item.releaseDate.empty() || item.seasonNumber > 0 ||
             item.episodeNumber > 0 || item.providerRuntimeMinutes > 0);
        item.metadataState = hasProvider && hasUsefulProvider
            ? "available"
            : (hasNative || hasProvider ? "partial" : "absent");

        return item;
    }

    // Read the same persisted TVScraper detail as the Web. Existing M1
    // facts remain unchanged if the richer native detail is not available.
    static void applyNativeDetails(
        const VdrRecording& source,
        const VdrRecordingNativeMetadataRecord& native,
        PublicRecordingMetadataDetail& item)
    {
        if (item.metadataSource == "manual" || !native.exists() ||
            !native.metadata.found || native.backendId != source.backendId ||
            native.backendNativeId != source.backendNativeId)
            return;
        const auto& value = native.metadata;
        // The existing Web detail prefers the persisted native detail over
        // the compact Recording cache. Preserve the separate VDR EPG facts.
        item.metadataState = "available";
        item.metadataSource = "restfulapi-scraper-bridge";
        if (value.mediaType == "movie") item.contentKind = "movie";
        else if (value.mediaType == "series") item.contentKind = "series";
        else if (value.mediaType == "episode") item.contentKind = "series-episode";
        if (!value.title.empty()) item.providerTitle = bounded(value.title, 1024U);
        if (!value.originalTitle.empty()) item.originalTitle = bounded(value.originalTitle, 1024U);
        if (!value.overview.empty())
            item.overview = value.overview == item.recording.description
                ? std::string{} : bounded(value.overview, 4096U);
        if (!value.tagline.empty()) item.tagline = bounded(value.tagline, 1024U);
        if (!value.episodeName.empty()) item.episodeTitle = bounded(value.episodeName, 1024U);
        if (!value.releaseDate.empty()) item.releaseDate = bounded(value.releaseDate, 32U);
        if (value.seasonNumber > 0) item.seasonNumber = value.seasonNumber;
        if (value.episodeNumber > 0) item.episodeNumber = value.episodeNumber;
        if (value.runtimeMinutes > 0) item.providerRuntimeMinutes = value.runtimeMinutes;
        // IMDb title IDs are public display metadata, not provider lookup
        // credentials or an alternative Recording identity.
        if (value.imdbId.size() > 2 && value.imdbId.size() <= 32 &&
            value.imdbId.compare(0, 2, "tt") == 0 &&
            std::all_of(value.imdbId.begin() + 2, value.imdbId.end(),
                [](unsigned char c) { return c >= '0' && c <= '9'; }))
            item.imdbId = value.imdbId;
        item.firstAired = bounded(value.firstAired, 32U);
        item.statusText = bounded(value.status, 1024U);
        item.collectionName = bounded(value.collectionName, 1024U);
        item.absoluteEpisodeNumber = std::max(0, value.absoluteEpisodeNumber);
        item.voteAverage = std::isfinite(value.voteAverage) &&
            value.voteAverage >= 0.0 && value.voteAverage <= 10.0
            ? value.voteAverage : 0.0;
        item.voteCount = std::max(0, value.voteCount);
        item.genres = boundedList(value.genres, VdrRecordingNativeMetadata::MaximumGenres);
        item.productionCountries = boundedList(value.productionCountries,
            VdrRecordingNativeMetadata::MaximumCountries);
        item.networks = boundedList(value.networks, VdrRecordingNativeMetadata::MaximumNetworks);
        item.people.clear();
        appendPeople(value.people, item);
    }

    // Existing Web-selected assignment takes precedence over automatically
    // scraped provider fields. Call ONLY with an assignment returned from the
    // existing backend/cache-key scoped, active-binding read-only repository.
    // Invalid/stale identity evidence fails closed instead of leaking raw IDs.
    static bool applyManualAssignment(
        const std::string& authorizedBackendId,
        const ManualRecordingMetadataAssignment& manual,
        PublicRecordingMetadataDetail& item)
    {
        if (!manual.found) return true;
        if (manual.backendId != authorizedBackendId ||
            !manual.relationshipLocked || manual.revision <= 0 ||
            !MetadataAssignmentId::isValidValue(manual.metadataAssignmentId) ||
            !MetadataEntityId::isValidValue(manual.metadataEntityId) ||
            !MetadataTargetId::isValidValue(manual.metadataTargetId) ||
            (manual.mediaType != "movie" && manual.mediaType != "series" &&
             manual.mediaType != "episode") ||
            manual.title.empty() || manual.title.size() > 4096U ||
            manual.seasonNumber < 0 || manual.episodeNumber < 0)
            return false;

        // Do not expose the manual repository's resourceKey, provider IDs,
        // external IDs, image paths, people or editing actor identity.
        item.metadataState = "available";
        item.metadataSource = "manual";
        item.contentKind = manual.mediaType == "episode"
            ? "series-episode" : manual.mediaType;
        item.manualAssignmentId = manual.metadataAssignmentId;
        item.metadataEntityId = manual.metadataEntityId;
        item.manualRevision = manual.revision;
        item.providerTitle = bounded(manual.title, 1024U);
        item.originalTitle = bounded(manual.originalTitle, 1024U);
        item.overview = bounded(manual.overview, 4096U);
        if (item.overview == item.recording.description)
            item.overview.clear();
        item.releaseDate = bounded(manual.releaseDate, 32U);
        item.seasonNumber = manual.seasonNumber;
        item.episodeNumber = manual.episodeNumber;
        item.seriesTitle = manual.mediaType == "series"
            ? item.providerTitle : "";
        item.episodeTitle = manual.mediaType == "episode"
            ? item.providerTitle : "";
        item.genreText.clear();
        item.tagline.clear();
        item.providerRuntimeMinutes = 0;
        // Manual assignment wins for cast and the optional Scraper facts too.
        item.firstAired.clear();
        item.statusText.clear();
        item.collectionName.clear();
        item.imdbId.clear();
        item.absoluteEpisodeNumber = 0;
        item.voteAverage = 0.0;
        item.voteCount = 0;
        item.genres.clear();
        item.productionCountries.clear();
        item.networks.clear();
        item.people.clear();
        appendPeople(manual.people, item);
        return true;
    }

private:
    static std::vector<std::string> boundedList(
        const std::vector<std::string>& values, std::size_t limit)
    {
        std::vector<std::string> result;
        for (const auto& value : values)
        {
            if (result.size() >= limit) break;
            if (!value.empty()) result.push_back(bounded(value, 1024U));
        }
        return result;
    }

    template <class Person>
    static void appendPeople(const std::vector<Person>& people,
                             PublicRecordingMetadataDetail& item)
    {
        for (std::size_t index = 0; index < people.size(); ++index)
        {
            if (item.people.size() >= VdrRecordingNativeMetadata::MaximumPeople)
                break;
            const auto& person = people[index];
            if (person.name.empty()) continue;
            item.people.push_back({bounded(person.role, 128U),
                bounded(person.name, 1024U),
                bounded(person.characterName, 1024U), index});
        }
    }

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

#include "PublicApiRuntime.h"

#include <mutex>
#include <utility>

// Existing Recording/Search/Genre read callback lifecycle on the single
// PublicApiRuntime instance. No new recording identity or cache authority.
void PublicApiRuntime::registerRecordingCollectionLookup(
    RecordingCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(recordingCollectionLookupMutex_);
    recordingCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetRecordingCollectionLookup()
{
    std::lock_guard<std::mutex> lock(recordingCollectionLookupMutex_);
    recordingCollectionLookup_ = {};
}

bool PublicApiRuntime::recordingCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(recordingCollectionLookupMutex_);
    return static_cast<bool>(recordingCollectionLookup_);
}

PublicRecordingCollectionResult PublicApiRuntime::lookupRecordingCollection(
    const PublicRecordingCollectionRequest& request) const
{
    RecordingCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(recordingCollectionLookupMutex_);
        lookup = recordingCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}


void PublicApiRuntime::registerRecordingDetailLookup(
    RecordingDetailLookup lookup)
{
    std::lock_guard<std::mutex> lock(recordingDetailLookupMutex_);
    recordingDetailLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetRecordingDetailLookup()
{
    std::lock_guard<std::mutex> lock(recordingDetailLookupMutex_);
    recordingDetailLookup_ = {};
}

PublicRecordingDetailResult PublicApiRuntime::lookupRecordingDetail(
    const PublicRecordingDetailRequest& request) const
{
    RecordingDetailLookup lookup;
    {
        std::lock_guard<std::mutex> lock(recordingDetailLookupMutex_);
        lookup = recordingDetailLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}

void PublicApiRuntime::registerRecordingArtworkLookup(
    RecordingArtworkLookup lookup)
{
    std::lock_guard<std::mutex> lock(recordingArtworkLookupMutex_);
    recordingArtworkLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetRecordingArtworkLookup()
{
    std::lock_guard<std::mutex> lock(recordingArtworkLookupMutex_);
    recordingArtworkLookup_ = {};
}

PublicRecordingArtworkResult PublicApiRuntime::lookupRecordingArtwork(
    const PublicRecordingArtworkRequest& request) const
{
    RecordingArtworkLookup lookup;
    {
        std::lock_guard<std::mutex> lock(recordingArtworkLookupMutex_);
        lookup = recordingArtworkLookup_;
    }
    return lookup ? lookup(request) : PublicRecordingArtworkResult{};
}

void PublicApiRuntime::registerSearchLookup(SearchLookup lookup)
{
    std::lock_guard<std::mutex> lock(searchLookupMutex_);
    searchLookup_ = std::move(lookup);
}
void PublicApiRuntime::resetSearchLookup()
{
    std::lock_guard<std::mutex> lock(searchLookupMutex_);
    searchLookup_ = {};
}
bool PublicApiRuntime::searchLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(searchLookupMutex_);
    return static_cast<bool>(searchLookup_);
}
PublicSearchResult PublicApiRuntime::lookupSearch(
    const PublicSearchRequest& request) const
{
    SearchLookup lookup;
    {
        std::lock_guard<std::mutex> lock(searchLookupMutex_);
        lookup = searchLookup_;
    }
    return lookup ? lookup(request) : PublicSearchResult{};
}

void PublicApiRuntime::registerEpgGenreLookup(EpgGenreLookup lookup)
{
    std::lock_guard<std::mutex> lock(epgGenreLookupMutex_);
    epgGenreLookup_ = std::move(lookup);
}
void PublicApiRuntime::resetEpgGenreLookup()
{
    std::lock_guard<std::mutex> lock(epgGenreLookupMutex_);
    epgGenreLookup_ = {};
}
bool PublicApiRuntime::epgGenreLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(epgGenreLookupMutex_);
    return static_cast<bool>(epgGenreLookup_);
}
PublicEpgGenreResult PublicApiRuntime::lookupEpgGenre(
    const PublicEpgGenreRequest& request) const
{
    EpgGenreLookup lookup;
    {
        std::lock_guard<std::mutex> lock(epgGenreLookupMutex_);
        lookup = epgGenreLookup_;
    }
    return lookup ? lookup(request) : PublicEpgGenreResult{};
}

void PublicApiRuntime::registerGenreCollectionLookup(
    GenreCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(genreCollectionLookupMutex_);
    genreCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetGenreCollectionLookup()
{
    std::lock_guard<std::mutex> lock(genreCollectionLookupMutex_);
    genreCollectionLookup_ = {};
}

bool PublicApiRuntime::genreCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(genreCollectionLookupMutex_);
    return static_cast<bool>(genreCollectionLookup_);
}

PublicGenreCollectionResult PublicApiRuntime::lookupGenreCollection(
    const PublicGenreCollectionRequest& request) const
{
    GenreCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(genreCollectionLookupMutex_);
        lookup = genreCollectionLookup_;
    }
    return lookup ? lookup(request) : PublicGenreCollectionResult{};
}


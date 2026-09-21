#pragma once

#include <string>
#include <vector>

enum class VdrRecordingMetadataSource
{
    None,
    RestfulApiScraperBridge
};

enum class VdrRecordingContentKind
{
    Unknown,
    Movie,
    SeriesEpisode
};

enum class VdrRecordingArtworkKind
{
    Poster,
    Fanart,
    Banner,
    Still
};

inline const char* vdrRecordingMetadataSourceName(
    const VdrRecordingMetadataSource source)
{
    switch (source)
    {
    case VdrRecordingMetadataSource::RestfulApiScraperBridge:
        return "restfulapi-scraper-bridge";
    case VdrRecordingMetadataSource::None:
    default:
        return "none";
    }
}

inline const char* vdrRecordingContentKindName(
    const VdrRecordingContentKind kind)
{
    switch (kind)
    {
    case VdrRecordingContentKind::Movie:
        return "movie";
    case VdrRecordingContentKind::SeriesEpisode:
        return "series-episode";
    case VdrRecordingContentKind::Unknown:
    default:
        return "unknown";
    }
}

inline const char* vdrRecordingArtworkKindName(
    const VdrRecordingArtworkKind kind)
{
    switch (kind)
    {
    case VdrRecordingArtworkKind::Poster:
        return "poster";
    case VdrRecordingArtworkKind::Fanart:
        return "fanart";
    case VdrRecordingArtworkKind::Banner:
        return "banner";
    case VdrRecordingArtworkKind::Still:
        return "still";
    default:
        return "poster";
    }
}

struct VdrRecordingEventMetadata
{
    std::string eventTitle;
    std::string shortText;
    std::string description;

    bool hasText() const
    {
        return !eventTitle.empty() ||
               !shortText.empty() ||
               !description.empty();
    }
};

struct VdrRecordingProviderMetadata
{
    VdrRecordingMetadataSource source =
        VdrRecordingMetadataSource::None;
    VdrRecordingContentKind contentKind =
        VdrRecordingContentKind::Unknown;

    std::string movieId;
    std::string seriesId;
    std::string episodeId;

    std::string title;
    std::string originalTitle;
    std::string tagline;
    std::string overview;
    std::string genreText;
    std::string releaseDate;

    std::string seriesTitle;
    std::string episodeTitle;
    int seasonNumber = 0;
    int episodeNumber = 0;
    int runtimeMinutes = 0;
    double rating = 0.0;

    bool hasData() const
    {
        return source != VdrRecordingMetadataSource::None;
    }
};

struct VdrRecordingArtworkRef
{
    VdrRecordingArtworkKind kind =
        VdrRecordingArtworkKind::Poster;
    VdrRecordingMetadataSource source =
        VdrRecordingMetadataSource::None;

    // This remains source-scoped cache evidence and is never a public URL.
    // The API derives a separate opaque Suite artwork identity from the
    // Recording and this reference before an authenticated delivery request.
    std::string reference;
    int width = 0;
    int height = 0;
    bool temporary = true;

    bool isValid() const
    {
        return source != VdrRecordingMetadataSource::None &&
               !reference.empty();
    }
};

struct VdrRecordingMetadata
{
    VdrRecordingEventMetadata native;
    VdrRecordingProviderMetadata provider;
    std::vector<VdrRecordingArtworkRef> artwork;

    bool hasProviderData() const
    {
        return provider.hasData();
    }

    bool hasArtwork() const
    {
        for (const VdrRecordingArtworkRef& item : artwork)
        {
            if (item.isValid())
            {
                return true;
            }
        }

        return false;
    }
};

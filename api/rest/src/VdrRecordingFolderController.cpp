#include "VdrRecordingFolderController.h"

#include "EpgArtworkController.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingMetadataJsonSerializer.h"
#include "VdrRecordingNativeMetadataPublicJsonSerializer.h"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

void appendJsonString(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';

    for (const char character : value)
    {
        switch (character)
        {
        case '"': json << "\\\""; break;
        case '\\': json << "\\\\"; break;
        case '\b': json << "\\b"; break;
        case '\f': json << "\\f"; break;
        case '\n': json << "\\n"; break;
        case '\r': json << "\\r"; break;
        case '\t': json << "\\t"; break;
        default:
            if (static_cast<unsigned char>(character) < 0x20)
            {
                json << "\\u00";
                const char* digits = "0123456789abcdef";
                json << digits[(character >> 4) & 0x0f];
                json << digits[character & 0x0f];
            }
            else json << character;
            break;
        }
    }

    json << '"';
}

ApiResponse jsonResponse(const std::string& body)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = body;
    return response;
}

ApiResponse jsonError(int statusCode, const std::string& message)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    std::ostringstream json;
    json << "{\"error\":";
    appendJsonString(json, message);
    json << "}";
    response.body = json.str();
    return response;
}

const VdrRecordingNativeArtwork* selectMetadataArtwork(
    const VdrRecordingNativeMetadata& metadata,
    const std::string& kind,
    int index)
{
    if (kind == "poster")
    {
        if (index != 0) return nullptr;
        for (const VdrRecordingNativeArtwork& artwork : metadata.images)
        {
            if (artwork.orientation == "portrait" &&
                artwork.available && !artwork.path.empty())
            {
                return &artwork;
            }
        }
        return &metadata.preferredArtwork;
    }
    if (kind == "preferred") return index == 0 ? &metadata.preferredArtwork : nullptr;
    if (kind == "person")
    {
        if (index < 0 || static_cast<std::size_t>(index) >= metadata.people.size())
            return nullptr;
        return &metadata.people[static_cast<std::size_t>(index)].image;
    }
    if (kind == "gallery")
    {
        if (index < 0 || static_cast<std::size_t>(index) >= metadata.images.size())
            return nullptr;
        return &metadata.images[static_cast<std::size_t>(index)];
    }
    return nullptr;
}

bool localManualImage(const std::string& path)
{
    if (path.empty()) return false;
    const std::filesystem::path normalized =
        std::filesystem::path(path).lexically_normal();
    return normalized.is_absolute() &&
        normalized != normalized.root_path() &&
        normalized.string().compare(
            0,
            std::string("/var/cache/vdr-suite/recording-metadata/posters/").size(),
            "/var/cache/vdr-suite/recording-metadata/posters/") == 0;
}

struct MetadataArtworkEntry
{
    VdrRecordingFolderController::PublicMetadataArtwork item;
    std::string path;
};

std::string metadataArtworkId(
    const VdrRecording& recording, const std::string& kind, int index,
    int revision, const std::string& path, int width, int height)
{
    VdrRecordingArtworkRef reference;
    reference.source = VdrRecordingMetadataSource::RestfulApiScraperBridge;
    reference.kind = VdrRecordingArtworkKind::Still;
    reference.reference = "metadata-image-v1:" + kind + ":" +
        std::to_string(index) + ":" + std::to_string(revision) + ":" + path;
    reference.width = std::max(0, width);
    reference.height = std::max(0, height);
    return VdrRecordingArtworkIdentity::assetId(recording, reference);
}

std::vector<MetadataArtworkEntry> metadataArtworkEntries(
    const VdrRecording& recording,
    const ManualRecordingMetadataAssignment& manual,
    const VdrRecordingNativeMetadataRecord& native)
{
    std::vector<MetadataArtworkEntry> entries;
    if (recording.backendId.empty() || recording.backendNativeId.empty())
        return entries;
    const auto add = [&](const std::string& kind, int index,
                         const std::string& path, const std::string& orientation,
                         int width, int height, int revision) {
        if (path.empty()) return;
        // Use the existing opaque asset identity algorithm with a disjoint
        // metadata source reference. Bind to the actual selected image, not
        // merely its mutable ordinal. Never expose the input or use it as URL.
        const auto id = metadataArtworkId(recording, kind, index, revision,
            path, width, height);
        if (!id.empty())
            entries.push_back({{id, kind, index, orientation,
                std::max(0, width), std::max(0, height)}, path});
    };
    if (manual.found && manual.relationshipLocked &&
        manual.backendId == recording.backendId)
    {
        if (localManualImage(manual.posterReference))
            add("preferred", 0, manual.posterReference, "portrait", 0, 0,
                manual.revision);
        for (std::size_t i = 0; i < manual.people.size(); ++i)
        {
            if (localManualImage(manual.people[i].profilePath))
                add("person", static_cast<int>(i), manual.people[i].profilePath,
                    "portrait", 0, 0, manual.revision);
        }
        return entries;
    }
    if (!native.exists() || !native.metadata.found ||
        native.backendId != recording.backendId ||
        native.backendNativeId != recording.backendNativeId)
        return entries;
    const auto addNative = [&](const std::string& kind, int index,
                               const VdrRecordingNativeArtwork& image) {
        if (image.available)
            add(kind, index, image.path, image.orientation,
                image.width, image.height, 0);
    };
    addNative("preferred", 0, native.metadata.preferredArtwork);
    for (std::size_t i = 0; i < native.metadata.people.size(); ++i)
        addNative("person", static_cast<int>(i), native.metadata.people[i].image);
    for (std::size_t i = 0; i < native.metadata.images.size(); ++i)
        addNative("gallery", static_cast<int>(i), native.metadata.images[i]);
    return entries;
}

std::string percentEncode(const std::string& value)
{
    static const char Hex[] = "0123456789ABCDEF";
    std::string output;
    for (const unsigned char character : value)
    {
        if ((character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9') ||
            character == '-' || character == '_' || character == '.' ||
            character == '~')
            output.push_back(static_cast<char>(character));
        else
        {
            output.push_back('%');
            output.push_back(Hex[character >> 4U]);
            output.push_back(Hex[character & 0x0fU]);
        }
    }
    return output;
}

std::string manualImageUrl(
    const std::string& backendId,
    const std::string& backendNativeId,
    const std::string& kind,
    int index,
    int revision)
{
    return "/api/vdr/recordings/metadata/image?backend=" +
        percentEncode(backendId) +
        "&backendNativeId=" + percentEncode(backendNativeId) +
        "&kind=" + percentEncode(kind) +
        "&index=" + std::to_string(index) +
        "&assignmentRevision=" + std::to_string(revision);
}

std::string manualContentKind(
    const ManualRecordingMetadataAssignment& assignment)
{
    if (assignment.mediaType == "movie") return "movie";
    if (assignment.mediaType == "series") return "series";
    if (assignment.mediaType == "episode") return "series-episode";
    return "unknown";
}

std::string manualSeasonEpisodeLabel(
    const ManualRecordingMetadataAssignment& assignment)
{
    std::ostringstream label;
    if (assignment.seasonNumber > 0)
    {
        label << 'S';
        if (assignment.seasonNumber < 10) label << '0';
        label << assignment.seasonNumber;
    }
    if (assignment.episodeNumber > 0)
    {
        label << 'E';
        if (assignment.episodeNumber < 10) label << '0';
        label << assignment.episodeNumber;
    }
    return label.str();
}

unsigned int manualPlaceholderVariant(const std::string& value)
{
    unsigned int hash = 2166136261u;
    for (const unsigned char character : value)
    {
        hash ^= character;
        hash *= 16777619u;
    }
    return hash % 6u;
}

void appendManualPeople(
    std::ostringstream& json,
    const ManualRecordingMetadataAssignment& assignment,
    const std::string& backendNativeId)
{
    json << '[';
    for (std::size_t index = 0; index < assignment.people.size(); ++index)
    {
        if (index > 0U) json << ',';
        const ManualRecordingMetadataPerson& person = assignment.people[index];
        const bool imageAvailable = localManualImage(person.profilePath);
        json << "{\"role\":";
        appendJsonString(json, person.role);
        json << ",\"name\":";
        appendJsonString(json, person.name);
        json << ",\"characterName\":";
        appendJsonString(json, person.characterName);
        json << ",\"image\":{\"available\":"
             << (imageAvailable ? "true" : "false");
        if (imageAvailable)
        {
            json << ",\"url\":";
            appendJsonString(
                json,
                manualImageUrl(
                    assignment.backendId,
                    backendNativeId,
                    "person",
                    static_cast<int>(index),
                    assignment.revision));
            json << ",\"width\":0,\"height\":0";
        }
        json << "}}";
    }
    json << ']';
}

std::string serializeManualFolderMetadata(
    const ManualRecordingMetadataAssignment& assignment,
    const std::string& backendNativeId)
{
    const bool posterAvailable = localManualImage(assignment.posterReference);
    const std::string posterUrl = posterAvailable
        ? manualImageUrl(
            assignment.backendId,
            backendNativeId,
            "preferred",
            0,
            assignment.revision)
        : std::string{};
    const std::string contentKind = manualContentKind(assignment);
    const std::string seasonEpisode = manualSeasonEpisodeLabel(assignment);

    std::ostringstream json;
    json << "{\"native\":{\"eventTitle\":\"\","
         << "\"shortText\":\"\",\"description\":\"\"},"
         << "\"provider\":{\"available\":true,\"source\":\"manual\","
         << "\"contentKind\":";
    appendJsonString(json, contentKind);
    json << ",\"movieId\":";
    appendJsonString(
        json,
        assignment.mediaType == "movie" ? assignment.externalId : std::string{});
    json << ",\"seriesId\":";
    appendJsonString(
        json,
        assignment.mediaType == "series" ? assignment.externalId : std::string{});
    json << ",\"episodeId\":";
    appendJsonString(
        json,
        assignment.mediaType == "episode" ? assignment.externalId : std::string{});
    json << ",\"title\":";
    appendJsonString(json, assignment.title);
    json << ",\"originalTitle\":";
    appendJsonString(json, assignment.originalTitle);
    json << ",\"tagline\":\"\",\"overview\":";
    appendJsonString(json, assignment.overview);
    json << ",\"genreText\":\"\",\"releaseDate\":";
    appendJsonString(json, assignment.releaseDate);
    json << ",\"seriesTitle\":";
    appendJsonString(
        json,
        assignment.mediaType == "series" ? assignment.title : std::string{});
    json << ",\"episodeTitle\":";
    appendJsonString(
        json,
        assignment.mediaType == "episode" ? assignment.title : std::string{});
    json << ",\"seasonNumber\":" << assignment.seasonNumber
         << ",\"episodeNumber\":" << assignment.episodeNumber
         << ",\"runtimeMinutes\":0,\"rating\":0}"
         << ",\"artwork\":{\"available\":"
         << (posterAvailable ? "true" : "false")
         << ",\"count\":" << (posterAvailable ? 1 : 0)
         << ",\"posterAvailable\":"
         << (posterAvailable ? "true" : "false")
         << ",\"fanartAvailable\":false,\"bannerAvailable\":false,"
         << "\"stillAvailable\":false,\"preferredAssetId\":\"\","
         << "\"preferredUrl\":";
    appendJsonString(json, posterUrl);
    json << "}"
         << ",\"presentation\":{\"title\":";
    appendJsonString(json, assignment.title);
    json << ",\"subtitle\":";
    appendJsonString(json, seasonEpisode);
    json << ",\"summary\":";
    appendJsonString(json, assignment.overview);
    json << ",\"contentKind\":";
    appendJsonString(json, contentKind);
    json << ",\"seasonEpisode\":";
    appendJsonString(json, seasonEpisode);
    json << ",\"posterAssetId\":\"\",\"posterUrl\":";
    appendJsonString(json, posterUrl);
    json << ",\"providerAvailable\":true,\"artworkPrepared\":"
         << (posterAvailable ? "true" : "false")
         << ",\"placeholderVariant\":"
         << manualPlaceholderVariant(assignment.title)
         << "}"
         << ",\"manualAssignment\":{\"active\":true,\"revision\":"
         << assignment.revision
         << ",\"relationshipLocked\":"
         << (assignment.relationshipLocked ? "true" : "false")
         << ",\"providerId\":";
    appendJsonString(json, assignment.providerId);
    json << ",\"externalNamespace\":";
    appendJsonString(json, assignment.externalNamespace);
    json << ",\"externalId\":";
    appendJsonString(json, assignment.externalId);
    json << "}}";
    return json.str();
}

// Match the Home recording-card poster precedence without serializing an
// entire episode (description, people, gallery) for a non-recording tile.
std::string representativePosterUrl(
    const VdrRecording& recording,
    const ManualRecordingMetadataAssignment& manual,
    const VdrRecordingNativeMetadataRecord& native)
{
    if (manual.found && manual.relationshipLocked)
        return localManualImage(manual.posterReference)
            ? manualImageUrl(manual.backendId, recording.backendNativeId,
                             "preferred", 0, manual.revision)
            : std::string{};
    if (native.exists() && native.metadata.found)
    {
        for (std::size_t i = 0; i < native.metadata.images.size(); ++i)
        {
            const auto& image = native.metadata.images[i];
            if (image.orientation == "portrait" && image.available && !image.path.empty())
                return VdrRecordingNativeMetadataPublicJsonSerializer::imageUrl(
                    native, "gallery", static_cast<int>(i));
        }
        const auto& preferred = native.metadata.preferredArtwork;
        if (preferred.available && !preferred.path.empty())
            return VdrRecordingNativeMetadataPublicJsonSerializer::imageUrl(native, "preferred", 0);
    }
    return VdrRecordingMetadataJsonSerializer::preferredArtworkUrl(recording);
}

void appendRecordingJson(
    std::ostringstream& json,
    const VdrRecording& recording,
    const ManualRecordingMetadataAssignment& manual,
    const VdrRecordingNativeMetadataRecord& nativeMetadata,
    const std::string& publicId = {})
{
    json << "{";
    if (!publicId.empty())
    {
        json << "\"publicRecordingId\":";
        appendJsonString(json, publicId);
        json << ",";
    }
    json << "\"id\":";
    appendJsonString(json, recording.id);
    json << ",\"backendId\":";
    appendJsonString(json, recording.backendId);
    json << ",\"backendNativeId\":";
    appendJsonString(json, recording.backendNativeId);
    json << ",\"title\":";
    appendJsonString(json, recording.title);
    json << ",\"path\":";
    appendJsonString(json, recording.path);
    json << ",\"startTime\":";
    appendJsonString(json, recording.startTime);
    json << ",\"durationSeconds\":" << recording.durationSeconds;
    json << ",\"sizeMb\":" << recording.sizeMb;
    json << ",\"metadata\":";

    if (manual.found && manual.relationshipLocked)
    {
        json << serializeManualFolderMetadata(
            manual,
            recording.backendNativeId);
    }
    else
    {
        json << VdrRecordingMetadataJsonSerializer::serialize(
            recording);
    }

    if (!(manual.found && manual.relationshipLocked) &&
        nativeMetadata.exists())
    {
        json << ",\"nativeMetadata\":"
             << VdrRecordingNativeMetadataPublicJsonSerializer()
                    .serialize(nativeMetadata);
    }

    json << "}";
}

std::string trimCachedMetadataText(const std::string& value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> cachedGenreLabels(const std::string& value)
{
    std::vector<std::string> result;
    std::string current;
    auto flush = [&result, &current]()
    {
        const std::string label = trimCachedMetadataText(current);
        if (!label.empty()) result.push_back(label);
        current.clear();
    };

    for (const char character : value)
    {
        if (character == '|' || character == ',' || character == ';')
            flush();
        else
            current.push_back(character);
    }
    flush();
    return result;
}

void appendCachedStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0) json << ',';
        appendJsonString(json, values[index]);
    }
    json << ']';
}

std::string cachedMediaType(
    const VdrRecordingProviderMetadata& provider)
{
    if (provider.contentKind == VdrRecordingContentKind::Movie)
        return "movie";
    if (provider.contentKind == VdrRecordingContentKind::SeriesEpisode)
        return "episode";
    return "unknown";
}

std::string cachedProviderName(
    const VdrRecordingProviderMetadata& provider)
{
    return provider.source == VdrRecordingMetadataSource::RestfulApiScraperBridge
        ? "tvscraper"
        : "recording-cache";
}

int cachedProviderId(
    const VdrRecordingProviderMetadata& provider)
{
    const std::string* value = nullptr;
    if (provider.contentKind == VdrRecordingContentKind::Movie)
        value = &provider.movieId;
    else if (provider.contentKind == VdrRecordingContentKind::SeriesEpisode)
        value = &provider.seriesId;
    if (value == nullptr || value->empty() || value->size() > 10U) return 0;

    long long parsed = 0;
    for (const unsigned char character : *value)
    {
        if (character < '0' || character > '9') return 0;
        parsed = parsed * 10LL + static_cast<long long>(character - '0');
        if (parsed > std::numeric_limits<int>::max()) return 0;
    }
    return parsed > 0 ? static_cast<int>(parsed) : 0;
}

std::string cachedArtworkOrientation(
    const VdrRecordingArtworkRef& artwork)
{
    switch (artwork.kind)
    {
    case VdrRecordingArtworkKind::Poster: return "portrait";
    case VdrRecordingArtworkKind::Banner: return "banner";
    case VdrRecordingArtworkKind::Fanart:
    case VdrRecordingArtworkKind::Still:
    default:
        return "landscape";
    }
}

bool cachedMetadataAvailable(const VdrRecording& recording)
{
    return recording.metadata.provider.hasData() ||
        recording.metadata.native.hasText() ||
        recording.metadata.hasArtwork();
}

void appendCachedArtwork(
    std::ostringstream& json,
    const VdrRecording& recording,
    const VdrRecordingArtworkRef* artwork)
{
    const std::string url =
        artwork != nullptr && artwork->isValid()
            ? VdrRecordingArtworkIdentity::publicUrl(recording, *artwork)
            : std::string{};
    const bool available = !url.empty();
    json << "{\"available\":" << (available ? "true" : "false");
    if (available)
    {
        json << ",\"url\":";
        appendJsonString(json, url);
        json << ",\"width\":" << artwork->width
             << ",\"height\":" << artwork->height;
    }
    json << '}';
}

std::string serializeCachedMetadata(const VdrRecording& recording)
{
    if (!cachedMetadataAvailable(recording)) return {};

    const VdrRecordingProviderMetadata& provider =
        recording.metadata.provider;
    const VdrRecordingEventMetadata& native =
        recording.metadata.native;
    const VdrRecordingArtworkRef* preferred =
        VdrRecordingArtworkIdentity::preferredArtwork(recording);

    const std::string title =
        !provider.title.empty()
            ? provider.title
            : (!provider.seriesTitle.empty()
                ? provider.seriesTitle
                : native.eventTitle);
    const std::string overview =
        !provider.overview.empty()
            ? provider.overview
            : native.description;
    const std::string tagline =
        !provider.tagline.empty()
            ? provider.tagline
            : native.shortText;

    std::ostringstream json;
    json << "{\"available\":true"
         << ",\"status\":\"ready\""
         << ",\"provider\":";
    appendJsonString(json, cachedProviderName(provider));
    json << ",\"mediaType\":";
    appendJsonString(json, cachedMediaType(provider));
    json << ",\"providerId\":" << cachedProviderId(provider)
         << ",\"seasonNumber\":" << provider.seasonNumber
         << ",\"episodeNumber\":" << provider.episodeNumber
         << ",\"absoluteEpisodeNumber\":0"
         << ",\"runtimeMinutes\":" << provider.runtimeMinutes
         << ",\"durationDeviationMinutes\":0"
         << ",\"popularity\":0"
         << ",\"voteAverage\":" << provider.rating
         << ",\"voteCount\":0"
         << ",\"adult\":false"
         << ",\"collectionId\":0"
         << ",\"lastSeason\":0"
         << ",\"title\":";
    appendJsonString(json, title);
    json << ",\"originalTitle\":";
    appendJsonString(json, provider.originalTitle);
    json << ",\"episodeName\":";
    appendJsonString(json, provider.episodeTitle);
    json << ",\"tagline\":";
    appendJsonString(json, tagline);
    json << ",\"overview\":";
    appendJsonString(json, overview);
    json << ",\"releaseDate\":";
    appendJsonString(json, provider.releaseDate);
    json << ",\"firstAired\":";
    appendJsonString(
        json,
        provider.contentKind == VdrRecordingContentKind::SeriesEpisode
            ? provider.releaseDate
            : std::string{});
    json << ",\"imdbId\":\"\""
         << ",\"statusText\":\"\""
         << ",\"collectionName\":\"\""
         << ",\"genres\":";
    appendCachedStringArray(json, cachedGenreLabels(provider.genreText));
    json << ",\"productionCountries\":[]"
         << ",\"networks\":[]"
         << ",\"providerHints\":{\"hd\":0,\"language\":-1}"
         << ",\"preferredArtwork\":";
    appendCachedArtwork(json, recording, preferred);
    json << ",\"people\":[]"
         << ",\"images\":[";

    bool firstImage = true;
    for (const VdrRecordingArtworkRef& artwork : recording.metadata.artwork)
    {
        if (!artwork.isValid()) continue;
        if (!firstImage) json << ',';
        firstImage = false;
        json << "{\"orientation\":";
        appendJsonString(json, cachedArtworkOrientation(artwork));
        json << ",\"image\":";
        appendCachedArtwork(json, recording, &artwork);
        json << '}';
    }

    json << "]}";
    return json.str();
}

std::string serializeManualMetadata(
    const ManualRecordingMetadataAssignment& assignment,
    const std::string& backendNativeId)
{
    const bool posterAvailable = localManualImage(assignment.posterReference);
    std::ostringstream json;
    json << "{\"available\":true,\"status\":\"ready\","
         << "\"provider\":\"manual\","
         << "\"mediaType\":";
    appendJsonString(json, assignment.mediaType);
    json << ",\"providerId\":0"
         << ",\"seasonNumber\":" << assignment.seasonNumber
         << ",\"episodeNumber\":" << assignment.episodeNumber
         << ",\"absoluteEpisodeNumber\":0"
         << ",\"runtimeMinutes\":0"
         << ",\"durationDeviationMinutes\":0"
         << ",\"popularity\":0"
         << ",\"voteAverage\":0"
         << ",\"voteCount\":0"
         << ",\"adult\":false"
         << ",\"collectionId\":0"
         << ",\"lastSeason\":0"
         << ",\"title\":";
    appendJsonString(json, assignment.title);
    json << ",\"originalTitle\":";
    appendJsonString(json, assignment.originalTitle);
    json << ",\"episodeName\":";
    appendJsonString(
        json,
        assignment.mediaType == "episode" ? assignment.title : std::string{});
    json << ",\"tagline\":\"\""
         << ",\"overview\":";
    appendJsonString(json, assignment.overview);
    json << ",\"releaseDate\":";
    appendJsonString(json, assignment.releaseDate);
    json << ",\"firstAired\":";
    appendJsonString(
        json,
        assignment.mediaType == "episode" ? assignment.releaseDate : std::string{});
    json << ",\"imdbId\":\"\""
         << ",\"statusText\":\"manual\""
         << ",\"collectionName\":\"\""
         << ",\"genres\":[]"
         << ",\"productionCountries\":[]"
         << ",\"networks\":[]"
         << ",\"providerHints\":{\"hd\":0,\"language\":-1}"
         << ",\"preferredArtwork\":{\"available\":"
         << (posterAvailable ? "true" : "false");
    if (posterAvailable)
    {
        json << ",\"url\":";
        appendJsonString(
            json,
            manualImageUrl(
                assignment.backendId,
                backendNativeId,
                "preferred",
                0,
                assignment.revision));
        json << ",\"width\":0,\"height\":0";
    }
    json << "}"
         << ",\"people\":";
    appendManualPeople(json, assignment, backendNativeId);
    json << ",\"images\":[]"
         << ",\"manualAssignment\":{"
         << "\"active\":true,\"revision\":" << assignment.revision
         << ",\"relationshipLocked\":"
         << (assignment.relationshipLocked ? "true" : "false")
         << ",\"providerId\":";
    appendJsonString(json, assignment.providerId);
    json << ",\"externalNamespace\":";
    appendJsonString(json, assignment.externalNamespace);
    json << ",\"externalId\":";
    appendJsonString(json, assignment.externalId);
    json << "}}";
    return json.str();
}

}

VdrRecordingFolderController::VdrRecordingFolderController(
    VdrRecordingCacheRepository& repository,
    NativeMetadataLookup nativeMetadataLookup,
    ManualMetadataLookup manualMetadataLookup,
    ManualMetadataBatchLookup manualMetadataBatchLookup,
    std::vector<std::string> metadataImageAllowedRoots)
    : repository_(repository),
      nativeMetadataLookup_(std::move(nativeMetadataLookup)),
      manualMetadataLookup_(std::move(manualMetadataLookup)),
      manualMetadataBatchLookup_(std::move(manualMetadataBatchLookup)),
      metadataImageAllowedRoots_(std::move(metadataImageAllowedRoots))
{
    if (metadataImageAllowedRoots_.empty())
    {
        metadataImageAllowedRoots_ = EpgArtworkController::defaultAllowedRoots();
    }
    metadataImageAllowedRoots_.push_back(
        "/var/cache/vdr-suite/recording-metadata/posters");
}

ApiResponse VdrRecordingFolderController::getStatus(
    const std::string& backendId)
{
    const VdrRecordingCacheStatus status = repository_.statusForBackend(backendId);
    std::ostringstream json;
    json << "{\"backendId\":";
    appendJsonString(json, status.backendId);
    json << ",\"state\":";
    appendJsonString(json, status.state);
    json << ",\"cacheReady\":" << (status.totalCount > 0 ? "true" : "false")
         << ",\"totalCount\":" << status.totalCount
         << ",\"startedAt\":";
    appendJsonString(json, status.startedAt);
    json << ",\"finishedAt\":";
    appendJsonString(json, status.finishedAt);
    json << ",\"lastError\":";
    appendJsonString(json, status.lastError);
    json << "}";
    return jsonResponse(json.str());
}

ApiResponse VdrRecordingFolderController::getFolder(
    const std::string& backendId,
    const std::string& path,
    int limit,
    int offset,
    bool homeCards)
{
    const VdrRecordingFolderPage page = repository_.folderPageForBackend(
        backendId, path, limit, offset, homeCards);
    const std::map<std::string, ManualRecordingMetadataAssignment>
        manualAssignments = manualMetadataBatchLookup_
            ? manualMetadataBatchLookup_(page.backendId)
            : std::map<std::string, ManualRecordingMetadataAssignment>{};
    auto manualFor = [this, &page, &manualAssignments](
        const VdrRecording& recording)
    {
        if (manualMetadataBatchLookup_)
        {
            for (const std::string* key : {
                     &recording.backendNativeId,
                     &recording.path,
                     &recording.id})
            {
                if (key->empty()) continue;
                const auto match = manualAssignments.find(*key);
                if (match != manualAssignments.end()) return match->second;
            }
            return ManualRecordingMetadataAssignment{};
        }

        if (!manualMetadataLookup_) return ManualRecordingMetadataAssignment{};
        return manualMetadataLookup_(
            recording.backendId.empty() ? page.backendId : recording.backendId,
            recording.backendNativeId);
    };

    auto nativeFor = [this, &page](
        const VdrRecording& recording)
    {
        if (!nativeMetadataLookup_ ||
            recording.backendNativeId.empty())
        {
            return VdrRecordingNativeMetadataRecord{};
        }

        return nativeMetadataLookup_(
            recording.backendId.empty()
                ? page.backendId
                : recording.backendId,
            recording.backendNativeId);
    };

    auto publicIdFor = [this, &page](const VdrRecording& recording)
    {
        if (!publicRecordingIdLookup_ || recording.backendNativeId.empty())
            return std::string{};
        return publicRecordingIdLookup_(page.backendId, recording.backendNativeId);
    };

    std::ostringstream json;
    json << "{\"recordingFolder\":true,\"backendId\":";
    appendJsonString(json, page.backendId);
    json << ",\"path\":";
    appendJsonString(json, page.path);
    json << ",\"parentPath\":";
    appendJsonString(json, page.parentPath);
    json << ",\"folderId\":";
    appendJsonString(json, page.path.empty() ? std::string{} :
        VdrRecordingCacheRepository::publicFolderId(page.backendId, page.path));
    json << ",\"cacheState\":";
    appendJsonString(json, page.cacheState);
    json << ",\"cacheReady\":" << (page.cacheReady ? "true" : "false")
         << ",\"totalCount\":" << page.totalCount
         << ",\"folderCount\":" << page.folderCount
         << ",\"recordingCount\":" << page.recordingCount
         << ",\"returnedCount\":" << page.recordings.size()
         << ",\"limit\":" << page.limit
         << ",\"offset\":" << page.offset;
    if (homeCards)
    {
        const auto cardCount = static_cast<long long>(page.folderCount) + page.recordingCount;
        const auto returned = page.folders.size() + page.recordings.size();
        json << ",\"homeCardPage\":true"
             << ",\"cardCount\":" << cardCount
             << ",\"returnedCards\":" << returned
             << ",\"hasMore\":"
             << (static_cast<long long>(page.offset) + static_cast<long long>(returned) < cardCount
                 ? "true" : "false");
    }
    json << ",\"folders\":[";
    for (std::size_t index = 0; index < page.folders.size(); ++index)
    {
        if (index > 0) json << ",";
        const auto& folder = page.folders.at(index);
        json << "{\"name\":";
        appendJsonString(json, folder.name);
        json << ",\"path\":";
        appendJsonString(json, folder.path);
        json << ",\"recordingCount\":" << folder.recordingCount
             << ",\"singleRecordingLeaf\":"
             << (folder.singleRecordingLeaf ? "true" : "false");
        json << ",\"folderId\":";
        appendJsonString(json, VdrRecordingCacheRepository::publicFolderId(
            page.backendId, folder.path));
        if (folder.singleRecordingLeaf)
        {
            json << ",\"singleRecording\":";
            const ManualRecordingMetadataAssignment manual =
                manualFor(folder.singleRecording);
            const VdrRecordingNativeMetadataRecord nativeMetadata =
                manual.found && manual.relationshipLocked
                    ? VdrRecordingNativeMetadataRecord{}
                    : nativeFor(folder.singleRecording);

            appendRecordingJson(
                json,
                folder.singleRecording,
                manual,
                nativeMetadata,
                publicIdFor(folder.singleRecording));
        }
        else if (homeCards && folder.hasRepresentativeRecording)
        {
            const auto& representative = folder.representativeRecording;
            const auto manual = manualFor(representative);
            const auto native = manual.found && manual.relationshipLocked
                ? VdrRecordingNativeMetadataRecord{} : nativeFor(representative);
            json << ",\"representativePosterUrl\":";
            appendJsonString(json, representativePosterUrl(representative, manual, native));
        }
        json << "}";
    }
    json << "],\"recordings\":[";
    for (std::size_t index = 0; index < page.recordings.size(); ++index)
    {
        if (index > 0) json << ",";
        const VdrRecording& recording =
            page.recordings.at(index);
        const ManualRecordingMetadataAssignment manual =
            manualFor(recording);
        const VdrRecordingNativeMetadataRecord nativeMetadata =
            manual.found && manual.relationshipLocked
                ? VdrRecordingNativeMetadataRecord{}
                : nativeFor(recording);

        appendRecordingJson(
            json,
            recording,
            manual,
            nativeMetadata,
            publicIdFor(recording));
    }
    json << "]}";
    return jsonResponse(json.str());
}

ApiResponse VdrRecordingFolderController::getRecordingForBrowser(
    const std::string& backendId,
    const std::string& publicRecordingId) const
{
    if (publicRecordingId.size() != 36U ||
        publicRecordingId.compare(0, 4, "rec_") != 0 ||
        !std::all_of(publicRecordingId.begin() + 4, publicRecordingId.end(),
            [](char c) { return (c >= '0' && c <= '9') ||
                                 (c >= 'a' && c <= 'f'); }))
        return jsonError(400, "invalid recordingId");
    if (!publicRecordingNativeLookup_)
        return jsonError(503, "recording identity unavailable");

    const auto nativeId = publicRecordingNativeLookup_(
        backendId, publicRecordingId);
    VdrRecording recording;
    if (nativeId.empty() ||
        !repository_.findByBackendNativeId(backendId, nativeId, recording) ||
        recording.backendId != backendId ||
        recording.backendNativeId != nativeId)
        return jsonError(404, "recording not found");

    const auto manual = manualMetadataLookup_
        ? manualMetadataLookup_(backendId, nativeId)
        : ManualRecordingMetadataAssignment{};
    const bool locked = manual.found && manual.relationshipLocked;
    const auto native = !locked && nativeMetadataLookup_
        ? nativeMetadataLookup_(backendId, nativeId)
        : VdrRecordingNativeMetadataRecord{};

    std::ostringstream json;
    json << "{\"publicRecordingId\":";
    appendJsonString(json, publicRecordingId);
    json << ",\"recording\":";
    appendRecordingJson(json, recording, manual, native, publicRecordingId);
    json << ",\"metadataDetail\":";
    if (locked)
        json << serializeManualMetadata(manual, nativeId);
    else if (native.exists() && native.metadata.found)
        json << VdrRecordingNativeMetadataPublicJsonSerializer().serialize(native);
    else
    {
        const auto cached = serializeCachedMetadata(recording);
        if (!cached.empty()) json << cached;
        else json << VdrRecordingNativeMetadataPublicJsonSerializer().serialize(native);
    }
    json << "}";
    return jsonResponse(json.str());
}

ApiResponse VdrRecordingFolderController::getMetadata(
    const std::string& backendId,
    const std::string& backendNativeId) const
{
    if (backendNativeId.empty() || backendNativeId.size() > 4096)
        return jsonError(400, "backendNativeId is required");

    if (manualMetadataLookup_)
    {
        const ManualRecordingMetadataAssignment manual =
            manualMetadataLookup_(backendId, backendNativeId);
        if (manual.found && manual.relationshipLocked)
            return jsonResponse(serializeManualMetadata(manual, backendNativeId));
    }

    VdrRecordingNativeMetadataRecord nativeMetadata;
    if (nativeMetadataLookup_)
    {
        nativeMetadata =
            nativeMetadataLookup_(backendId, backendNativeId);
        if (nativeMetadata.exists() && nativeMetadata.metadata.found)
        {
            return jsonResponse(
                VdrRecordingNativeMetadataPublicJsonSerializer().serialize(
                    nativeMetadata));
        }
    }

    VdrRecording cachedRecording;
    if (repository_.findByBackendNativeId(
            backendId,
            backendNativeId,
            cachedRecording))
    {
        const std::string cachedMetadata =
            serializeCachedMetadata(cachedRecording);
        if (!cachedMetadata.empty())
            return jsonResponse(cachedMetadata);
    }

    if (!nativeMetadataLookup_)
        return jsonError(503, "recording metadata unavailable");

    return jsonResponse(
        VdrRecordingNativeMetadataPublicJsonSerializer().serialize(
            nativeMetadata));
}

std::vector<VdrRecordingFolderController::PublicMetadataArtwork>
VdrRecordingFolderController::publicMetadataArtwork(
    const VdrRecording& recording,
    const ManualRecordingMetadataAssignment& manual,
    const VdrRecordingNativeMetadataRecord& native)
{
    std::vector<PublicMetadataArtwork> result;
    for (const auto& entry : metadataArtworkEntries(recording, manual, native))
        result.push_back(entry.item);
    return result;
}

std::string VdrRecordingFolderController::publicSearchPreferredArtworkId(
    const VdrRecording& recording, const std::string& localPath,
    int width, int height, bool manualSelected, int manualRevision)
{
    if (recording.backendId.empty() || recording.backendNativeId.empty()) return {};
    if (manualSelected)
    {
        if (manualRevision <= 0 || !localManualImage(localPath)) return {};
        return metadataArtworkId(recording, "preferred", 0, manualRevision,
            localPath, 0, 0);
    }
    if (!localPath.empty())
        return metadataArtworkId(recording, "preferred", 0, 0,
            localPath, width, height);
    const auto* cached = VdrRecordingArtworkIdentity::preferredArtwork(recording);
    return cached ? VdrRecordingArtworkIdentity::assetId(recording, *cached) : std::string{};
}

std::string VdrRecordingFolderController::publicNativeCardArtworkId(
    const VdrRecording& recording, const std::string& path,
    int width, int height, int galleryIndex)
{
    if (recording.backendId.empty() || recording.backendNativeId.empty() || path.empty()) return {};
    return metadataArtworkId(recording, galleryIndex >= 0 ? "gallery" : "preferred",
        std::max(0, galleryIndex), 0, path, width, height);
}

std::string VdrRecordingFolderController::publicManualPortraitArtworkId(
    const VdrRecording& recording, int index, int revision,
    const std::string& localPath)
{
    if (recording.backendId.empty() || recording.backendNativeId.empty() ||
        index < 0 || revision <= 0 || !localManualImage(localPath))
        return {};
    return metadataArtworkId(recording, "person", index, revision,
        localPath, 0, 0);
}

std::optional<ApiResponse> VdrRecordingFolderController::loadPublicMetadataArtwork(
    const VdrRecording& recording,
    const ManualRecordingMetadataAssignment& manual,
    const VdrRecordingNativeMetadataRecord& native,
    const std::string& artworkId) const
{
    for (const auto& entry : metadataArtworkEntries(recording, manual, native))
    {
        if (entry.item.artworkId == artworkId)
            return EpgArtworkController::serveValidatedPath(
                entry.path, metadataImageAllowedRoots_);
    }
    return std::nullopt;
}

std::pair<std::string, std::string>
VdrRecordingFolderController::metadataTrailerIdentity(
    const VdrRecording& recording,
    const ManualRecordingMetadataAssignment& manual,
    const VdrRecordingNativeMetadataRecord& native)
{
    const auto valid = [](const std::string& id) {
        return !id.empty() && id.size() <= 16U &&
            id.front() >= '1' && id.front() <= '9' &&
            std::all_of(id.begin(), id.end(),
                [](unsigned char c) { return c >= '0' && c <= '9'; });
    };
    if (manual.found && manual.relationshipLocked &&
        manual.backendId == recording.backendId)
    {
        if (manual.providerId == "tmdb" && valid(manual.externalId))
        {
            if (manual.externalNamespace == "movie")
                return {"movie", manual.externalId};
            if (manual.externalNamespace == "tv")
                return {"series", manual.externalId};
        }
        // A selected episode ID is not a parent series ID.
        return {};
    }
    if (native.exists() && native.metadata.found &&
        native.backendId == recording.backendId &&
        native.backendNativeId == recording.backendNativeId)
    {
        const auto& value = native.metadata;
        if (value.provider != "tvscraper" && value.provider != "recording-cache")
            return {};
        if (value.providerId <= 0) return {};
        if (value.mediaType == "movie")
            return {"movie", std::to_string(value.providerId)};
        if (value.mediaType == "series" || value.mediaType == "episode")
            return {"series", std::to_string(value.providerId)};
        return {};
    }
    const auto& provider = recording.metadata.provider;
    const int id = cachedProviderId(provider);
    if (id <= 0) return {};
    if (provider.contentKind == VdrRecordingContentKind::Movie)
        return {"movie", std::to_string(id)};
    if (provider.contentKind == VdrRecordingContentKind::SeriesEpisode)
        return {"series", std::to_string(id)};
    return {};
}

ApiResponse VdrRecordingFolderController::getMetadataImage(
    const std::string& backendId,
    const std::string& backendNativeId,
    const std::string& kind,
    int index) const
{
    if (backendNativeId.empty() || backendNativeId.size() > 4096 ||
        kind.empty() || index < 0)
        return jsonError(
            400,
            "backendNativeId, kind and non-negative index are required");

    if (kind != "poster" && kind != "preferred" &&
        kind != "person" && kind != "gallery")
        return jsonError(400, "unsupported recording metadata image kind");

    if (manualMetadataLookup_)
    {
        const ManualRecordingMetadataAssignment manual =
            manualMetadataLookup_(backendId, backendNativeId);
        if (manual.found && manual.relationshipLocked)
        {
            std::string localPath;
            if ((kind == "poster" || kind == "preferred") && index == 0)
                localPath = manual.posterReference;
            else if (kind == "person" &&
                     static_cast<std::size_t>(index) < manual.people.size())
                localPath = manual.people[static_cast<std::size_t>(index)].profilePath;

            if (localManualImage(localPath))
            {
                return EpgArtworkController::serveValidatedPath(
                    localPath,
                    metadataImageAllowedRoots_);
            }
            if (kind == "poster")
                return jsonError(404, "recording metadata image not found");
        }
    }

    if (!nativeMetadataLookup_)
        return jsonError(503, "recording metadata unavailable");

    const VdrRecordingNativeMetadataRecord record =
        nativeMetadataLookup_(backendId, backendNativeId);
    if (!record.exists() || !record.metadata.found)
        return jsonError(404, "recording metadata not found");

    const VdrRecordingNativeArtwork* artwork =
        selectMetadataArtwork(record.metadata, kind, index);
    if (artwork == nullptr || !artwork->available || artwork->path.empty())
        return jsonError(404, "recording metadata image not found");

    return EpgArtworkController::serveValidatedPath(
        artwork->path,
        metadataImageAllowedRoots_);
}

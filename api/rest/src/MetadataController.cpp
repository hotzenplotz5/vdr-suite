#include "MetadataController.h"

#include "CurlExternalArtworkHttpTransport.h"
#include "MetadataRepository.h"
#include "TmdbRecordingMetadataCandidateProvider.h"
#include "TmdbRecordingMetadataCredentialResolver.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>

namespace
{
std::string environmentOrEmpty(const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string(value);
}

std::string environmentOrDefault(
    const char* name,
    const std::string& fallback)
{
    const std::string value = environmentOrEmpty(name);
    return value.empty() ? fallback : value;
}

TmdbRecordingMetadataCandidateProviderConfig providerConfig(
    const std::string& backendId)
{
    TmdbRecordingMetadataCandidateProviderConfig config;
    config.readAccessToken =
        TmdbRecordingMetadataCredentialResolver::resolveReadAccessToken(
            backendId);
    config.language = environmentOrDefault(
        "VDR_SUITE_TMDB_LANGUAGE",
        config.language);
    return config;
}

CurlExternalArtworkHttpTransport& defaultTransport()
{
    static CurlExternalArtworkHttpTransport transport(
        CurlExternalArtworkHttpTransportConfig{
            {
                "api.themoviedb.org",
                "image.tmdb.org"
            },
            "vdr-suite/manual-recording-metadata"
        });
    return transport;
}

std::string jsonEscape(const std::string& value)
{
    static const char Hex[] = "0123456789abcdef";
    std::string escaped;
    escaped.reserve(value.size() + 16U);
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (character < 0x20U)
            {
                escaped += "\\u00";
                escaped.push_back(Hex[(character >> 4U) & 0x0fU]);
                escaped.push_back(Hex[character & 0x0fU]);
            }
            else escaped.push_back(static_cast<char>(character));
        }
    }
    return escaped;
}

ApiResponse errorResponse(
    int statusCode,
    const std::string& code,
    const std::string& message)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) +
        "\",\"message\":\"" + jsonEscape(message) + "\"}}";
    return response;
}

std::string serializeCandidate(
    const RecordingMetadataCandidate& candidate)
{
    std::ostringstream json;
    json << "{";
    json << "\"kind\":\""
         << recordingMetadataCandidateKindName(candidate.kind) << "\",";
    json << "\"providerId\":\"" << jsonEscape(candidate.providerId) << "\",";
    json << "\"externalNamespace\":\""
         << jsonEscape(candidate.externalNamespace) << "\",";
    json << "\"externalId\":\"" << jsonEscape(candidate.externalId) << "\",";
    json << "\"parentExternalId\":\""
         << jsonEscape(candidate.parentExternalId) << "\",";
    json << "\"title\":\"" << jsonEscape(candidate.title) << "\",";
    json << "\"originalTitle\":\""
         << jsonEscape(candidate.originalTitle) << "\",";
    json << "\"overview\":\"" << jsonEscape(candidate.overview) << "\",";
    json << "\"releaseDate\":\""
         << jsonEscape(candidate.releaseDate) << "\",";
    json << "\"posterReference\":\""
         << jsonEscape(candidate.posterReference) << "\",";
    json << "\"seasonNumber\":" << candidate.seasonNumber << ",";
    json << "\"episodeNumber\":" << candidate.episodeNumber << ",";
    json << "\"rating\":" << candidate.rating;
    json << "}";
    return json.str();
}

ApiResponse candidateResponse(const RecordingMetadataCandidatePage& page)
{
    if (!page.error.empty())
    {
        const int status = page.providerAvailable ? 502 : 503;
        return errorResponse(
            status,
            page.attempted
                ? "metadata_provider_unavailable"
                : "metadata_provider_not_configured",
            page.error);
    }

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    std::ostringstream json;
    json << "{\"providerId\":\"" << jsonEscape(page.providerId) << "\",";
    json << "\"attempted\":" << (page.attempted ? "true" : "false") << ",";
    json << "\"providerAvailable\":"
         << (page.providerAvailable ? "true" : "false") << ",";
    json << "\"truncated\":" << (page.truncated ? "true" : "false") << ",";
    json << "\"count\":" << page.candidates.size() << ",";
    json << "\"candidates\":[";
    for (std::size_t index = 0; index < page.candidates.size(); ++index)
    {
        if (index > 0) json << ",";
        json << serializeCandidate(page.candidates[index]);
    }
    json << "]}";
    response.body = json.str();
    return response;
}

std::string serializeAssignment(
    const ManualRecordingMetadataAssignment& assignment)
{
    std::ostringstream json;
    json << "{";
    json << "\"found\":" << (assignment.found ? "true" : "false");
    if (assignment.found)
    {
        json << ",\"backendId\":\"" << jsonEscape(assignment.backendId) << "\"";
        json << ",\"metadataTargetId\":\""
             << jsonEscape(assignment.metadataTargetId) << "\"";
        json << ",\"metadataAssignmentId\":\""
             << jsonEscape(assignment.metadataAssignmentId) << "\"";
        json << ",\"metadataEntityId\":\""
             << jsonEscape(assignment.metadataEntityId) << "\"";
        json << ",\"providerId\":\"" << jsonEscape(assignment.providerId) << "\"";
        json << ",\"externalNamespace\":\""
             << jsonEscape(assignment.externalNamespace) << "\"";
        json << ",\"externalId\":\"" << jsonEscape(assignment.externalId) << "\"";
        json << ",\"mediaType\":\"" << jsonEscape(assignment.mediaType) << "\"";
        json << ",\"title\":\"" << jsonEscape(assignment.title) << "\"";
        json << ",\"originalTitle\":\""
             << jsonEscape(assignment.originalTitle) << "\"";
        json << ",\"overview\":\"" << jsonEscape(assignment.overview) << "\"";
        json << ",\"releaseDate\":\""
             << jsonEscape(assignment.releaseDate) << "\"";
        json << ",\"posterAvailable\":"
             << (!assignment.posterReference.empty() ? "true" : "false");
        json << ",\"seasonNumber\":" << assignment.seasonNumber;
        json << ",\"episodeNumber\":" << assignment.episodeNumber;
        json << ",\"revision\":" << assignment.revision;
        json << ",\"manual\":true";
        json << ",\"relationshipLocked\":"
             << (assignment.relationshipLocked ? "true" : "false");
    }
    json << "}";
    return json.str();
}
}

MetadataController::MetadataController(
    MetadataRepository& metadataRepository,
    IRecordingMetadataCandidateProvider* candidateProvider)
    : metadataRepository_(metadataRepository),
      candidateProvider_(candidateProvider)
{
}

ApiResponse MetadataController::getMetadata()
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";

    const auto metadataItems = metadataRepository_.getAllMetadata();
    std::ostringstream json;
    json << "{\"metadata\":[";
    for (std::size_t index = 0; index < metadataItems.size(); ++index)
    {
        const auto& metadata = metadataItems[index];
        if (index > 0) json << ",";
        json << "{";
        json << "\"id\":" << metadata.id << ",";
        json << "\"recordingId\":" << metadata.recordingId << ",";
        json << "\"mediaType\":\"" << jsonEscape(metadata.mediaType) << "\",";
        json << "\"title\":\"" << jsonEscape(metadata.title) << "\",";
        json << "\"originalTitle\":\""
             << jsonEscape(metadata.originalTitle) << "\",";
        json << "\"year\":" << metadata.year << ",";
        json << "\"seasonNumber\":" << metadata.seasonNumber << ",";
        json << "\"episodeNumber\":" << metadata.episodeNumber << ",";
        json << "\"genre\":\"" << jsonEscape(metadata.genre) << "\",";
        json << "\"description\":\""
             << jsonEscape(metadata.description) << "\",";
        json << "\"source\":\"" << jsonEscape(metadata.source) << "\",";
        json << "\"externalId\":\""
             << jsonEscape(metadata.externalId) << "\"";
        json << "}";
    }
    json << "]}";
    response.body = json.str();
    return response;
}

ApiResponse MetadataController::searchRecordingMetadataCandidates(
    const std::string& backendId,
    const std::string& query,
    RecordingMetadataCandidateKind kind,
    int limit)
{
    if (candidateProvider_ != nullptr)
        return candidateResponse(candidateProvider_->search(query, kind, limit));

    TmdbRecordingMetadataCandidateProvider provider(
        defaultTransport(),
        providerConfig(backendId));
    return candidateResponse(provider.search(query, kind, limit));
}

ApiResponse MetadataController::getRecordingMetadataSeasons(
    const std::string& backendId,
    const std::string& seriesExternalId,
    int limit)
{
    if (candidateProvider_ != nullptr)
        return candidateResponse(candidateProvider_->seasons(
            seriesExternalId,
            limit));

    TmdbRecordingMetadataCandidateProvider provider(
        defaultTransport(),
        providerConfig(backendId));
    return candidateResponse(provider.seasons(seriesExternalId, limit));
}

ApiResponse MetadataController::getRecordingMetadataEpisodes(
    const std::string& backendId,
    const std::string& seriesExternalId,
    int seasonNumber,
    int limit)
{
    if (candidateProvider_ != nullptr)
        return candidateResponse(candidateProvider_->episodes(
            seriesExternalId,
            seasonNumber,
            limit));

    TmdbRecordingMetadataCandidateProvider provider(
        defaultTransport(),
        providerConfig(backendId));
    return candidateResponse(provider.episodes(
        seriesExternalId,
        seasonNumber,
        limit));
}

ApiResponse MetadataController::getManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = serializeAssignment(
        metadataRepository_.getManualRecordingMetadata(backendId, resourceKey));
    return response;
}

ApiResponse MetadataController::assignManualRecordingMetadata(
    ManualRecordingMetadataSelection selection,
    const std::string& actorRef)
{
    selection.actorRef = actorRef;
    ManualRecordingMetadataAssignment assigned;
    if (!metadataRepository_.assignManualRecordingMetadata(selection, assigned))
    {
        return errorResponse(
            selection.expectedRevision > 0 ? 409 : 400,
            selection.expectedRevision > 0
                ? "metadata_assignment_revision_conflict"
                : "invalid_metadata_assignment",
            selection.expectedRevision > 0
                ? "The selected metadata assignment changed before this request"
                : "The manual metadata assignment is invalid");
    }

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = serializeAssignment(assigned);
    return response;
}

ApiResponse MetadataController::withdrawManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey,
    int expectedRevision,
    const std::string& actorRef)
{
    ManualRecordingMetadataAssignment withdrawn;
    if (!metadataRepository_.withdrawManualRecordingMetadata(
            backendId,
            resourceKey,
            actorRef,
            expectedRevision,
            withdrawn))
    {
        return errorResponse(
            409,
            "metadata_assignment_revision_conflict",
            "The selected metadata assignment no longer matches this request");
    }

    withdrawn.found = true;
    withdrawn.relationshipLocked = false;
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = serializeAssignment(withdrawn);
    return response;
}

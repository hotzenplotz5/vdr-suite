#include "MetadataController.h"

#include "MetadataRepository.h"

#include <sstream>

MetadataController::MetadataController(
    MetadataRepository& metadataRepository)
    : metadataRepository_(metadataRepository)
{
}

ApiResponse MetadataController::getMetadata()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";

    const auto metadataItems =
        metadataRepository_.getAllMetadata();

    std::ostringstream json;

    json << "{";
    json << "\"metadata\":[";

    for (std::size_t i = 0; i < metadataItems.size(); ++i)
    {
        const auto& metadata = metadataItems[i];

        json << "{";
        json << "\"id\":" << metadata.id << ",";
        json << "\"recordingId\":" << metadata.recordingId << ",";
        json << "\"mediaType\":\"" << metadata.mediaType << "\",";
        json << "\"title\":\"" << metadata.title << "\",";
        json << "\"originalTitle\":\"" << metadata.originalTitle << "\",";
        json << "\"year\":" << metadata.year << ",";
        json << "\"seasonNumber\":" << metadata.seasonNumber << ",";
        json << "\"episodeNumber\":" << metadata.episodeNumber << ",";
        json << "\"genre\":\"" << metadata.genre << "\",";
        json << "\"description\":\"" << metadata.description << "\",";
        json << "\"source\":\"" << metadata.source << "\",";
        json << "\"externalId\":\"" << metadata.externalId << "\"";
        json << "}";
        
        if (i + 1 < metadataItems.size())
        {
            json << ",";
        }
    }

    json << "]";
    json << "}";

    response.body = json.str();

    return response;
}

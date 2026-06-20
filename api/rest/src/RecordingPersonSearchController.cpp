#include "RecordingPersonSearchController.h"

#include "PersonQuery.h"
#include "RecordingPersonSearchResult.h"
#include "RecordingPersonSearchResultJsonSerializer.h"
#include "RecordingPersonSearchService.h"

namespace {

ApiResponse makeJsonResponse(
    const std::string& body)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = body;

    return response;
}

ApiResponse makeBadRequestResponse(
    const std::string& message)
{
    ApiResponse response;

    response.statusCode = 400;
    response.contentType = "application/json";
    response.body = "{\"error\":\"" + message + "\"}";

    return response;
}

bool parseRole(
    const std::string& value,
    PersonRole& role)
{
    if (value.empty()) {
        return true;
    }

    if (value == "unknown") {
        role = PersonRole::Unknown;
        return true;
    }

    if (value == "actor") {
        role = PersonRole::Actor;
        return true;
    }

    if (value == "director") {
        role = PersonRole::Director;
        return true;
    }

    if (value == "writer") {
        role = PersonRole::Writer;
        return true;
    }

    if (value == "producer") {
        role = PersonRole::Producer;
        return true;
    }

    if (value == "moderator") {
        role = PersonRole::Moderator;
        return true;
    }

    if (value == "guest") {
        role = PersonRole::Guest;
        return true;
    }

    if (value == "composer") {
        role = PersonRole::Composer;
        return true;
    }

    if (value == "other") {
        role = PersonRole::Other;
        return true;
    }

    return false;
}

bool parseSource(
    const std::string& value,
    ContentClassificationSource& source)
{
    if (value.empty()) {
        return true;
    }

    if (value == "epg") {
        source = ContentClassificationSource::Epg;
        return true;
    }

    if (value == "dvb") {
        source = ContentClassificationSource::Dvb;
        return true;
    }

    if (value == "tvscraper") {
        source = ContentClassificationSource::Tvscraper;
        return true;
    }

    if (value == "scraper2vdr") {
        source = ContentClassificationSource::Scraper2Vdr;
        return true;
    }

    if (value == "tmdb") {
        source = ContentClassificationSource::Tmdb;
        return true;
    }

    if (value == "tvdb") {
        source = ContentClassificationSource::Tvdb;
        return true;
    }

    if (value == "imdb") {
        source = ContentClassificationSource::Imdb;
        return true;
    }

    if (value == "user") {
        source = ContentClassificationSource::User;
        return true;
    }

    if (value == "folder") {
        source = ContentClassificationSource::Folder;
        return true;
    }

    if (value == "derived") {
        source = ContentClassificationSource::Derived;
        return true;
    }

    return false;
}

}

RecordingPersonSearchController::RecordingPersonSearchController(
    RecordingPersonSearchService& searchService,
    RecordingPersonSearchResultJsonSerializer& jsonSerializer)
    : searchService_(searchService),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse RecordingPersonSearchController::searchRecordingPersons(
    const std::vector<VdrRecording>& recordings,
    const std::string& name,
    const std::string& normalizedName,
    const std::string& characterName,
    const std::string& role,
    const std::string& source,
    const std::string& providerReference,
    int limit,
    int offset)
{
    if (limit < 0) {
        return makeBadRequestResponse("limit must not be negative");
    }

    if (offset < 0) {
        return makeBadRequestResponse("offset must not be negative");
    }

    PersonQuery query =
        PersonQuery::createEmpty();

    if (!name.empty()) {
        query.withName(name);
    }

    if (!normalizedName.empty()) {
        query.withNormalizedName(normalizedName);
    }

    if (!characterName.empty()) {
        query.withCharacterName(characterName);
    }

    if (!providerReference.empty()) {
        query.withProviderReference(providerReference);
    }

    if (!role.empty()) {
        PersonRole parsedRole = PersonRole::Unknown;

        if (!parseRole(
            role,
            parsedRole)) {
            return makeBadRequestResponse("invalid person role");
        }

        query.withRole(parsedRole);
    }

    if (!source.empty()) {
        ContentClassificationSource parsedSource =
            ContentClassificationSource::Derived;

        if (!parseSource(
            source,
            parsedSource)) {
            return makeBadRequestResponse("invalid person source");
        }

        query.withSource(parsedSource);
    }

    const RecordingPersonSearchResult result =
        searchService_.search(
            recordings,
            query,
            limit,
            offset);

    return makeJsonResponse(
        jsonSerializer_.serialize(result));
}

#include "ContentRatingResolutionJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    ContentRatingResolutionJsonSerializer serializer;

    std::string unresolvedJson =
        serializer.serialize(ContentRatingResolutionResult::unresolved());

    assert(unresolvedJson.find("\"resolved\":false") != std::string::npos);
    assert(unresolvedJson.find("\"primaryRating\":null") != std::string::npos);
    assert(unresolvedJson.find("\"evidence\":[]") != std::string::npos);

    ContentRatingCollection ratings = ContentRatingCollection::createEmpty();

    ratings.add(
        ContentRating::from(
            ContentClassificationSource::Dvb,
            ContentRatingSystem::Fsk,
            "FSK 12",
            12));

    ratings.add(
        ContentRating::withProviderReference(
            ContentClassificationSource::Tmdb,
            ContentRatingSystem::ProviderSpecific,
            "DE:16",
            16,
            90,
            "tmdb:release-certification:de"));

    ContentRatingResolver resolver;
    ContentRatingResolutionResult resolved = resolver.resolve(ratings);

    std::string json = serializer.serialize(resolved);

    assert(json.find("\"resolved\":true") != std::string::npos);
    assert(json.find("\"primaryRating\"") != std::string::npos);
    assert(json.find("\"source\":\"tmdb\"") != std::string::npos);
    assert(json.find("\"source\":\"dvb\"") != std::string::npos);
    assert(json.find("\"system\":\"provider-specific\"") != std::string::npos);
    assert(json.find("\"system\":\"fsk\"") != std::string::npos);
    assert(json.find("\"originalValue\":\"DE:16\"") != std::string::npos);
    assert(json.find("\"minimumAge\":16") != std::string::npos);
    assert(json.find("\"confidence\":90") != std::string::npos);
    assert(json.find("\"providerReference\":\"tmdb:release-certification:de\"") != std::string::npos);

    ContentRatingCollection escapedRatings = ContentRatingCollection::createEmpty();

    escapedRatings.add(
        ContentRating::from(
            ContentClassificationSource::User,
            ContentRatingSystem::UserDefined,
            "Manual \"Teen\"",
            12));

    std::string escapedJson =
        serializer.serialize(resolver.resolve(escapedRatings));

    assert(escapedJson.find("Manual \\\"Teen\\\"") != std::string::npos);

    std::cout << "test_content_rating_resolution_json_serializer passed" << std::endl;
    return 0;
}

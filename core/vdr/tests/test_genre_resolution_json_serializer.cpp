#include "GenreResolutionJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    GenreResolutionJsonSerializer serializer;

    std::string unresolvedJson =
        serializer.serialize(
            GenreResolutionResult::unresolved());

    assert(unresolvedJson.find("\"resolved\":false") != std::string::npos);
    assert(unresolvedJson.find("\"primaryGenre\":null") != std::string::npos);
    assert(unresolvedJson.find("\"evidence\":[]") != std::string::npos);

    GenreCollection collection = GenreCollection::createEmpty();

    collection.add(
        GenreClassification::from(
            ContentClassificationSource::Dvb,
            "Krimi",
            "crime"));

    collection.add(
        GenreClassification::withProviderReference(
            ContentClassificationSource::Tmdb,
            "Crime",
            "crime",
            90,
            "tmdb:80"));

    GenreResolver resolver;
    GenreResolutionResult resolved = resolver.resolve(collection);

    std::string resolvedJson = serializer.serialize(resolved);

    assert(resolvedJson.find("\"resolved\":true") != std::string::npos);
    assert(resolvedJson.find("\"primaryGenre\"") != std::string::npos);
    assert(resolvedJson.find("\"canonicalId\":\"crime\"") != std::string::npos);
    assert(resolvedJson.find("\"source\":\"tmdb\"") != std::string::npos);
    assert(resolvedJson.find("\"source\":\"dvb\"") != std::string::npos);
    assert(resolvedJson.find("\"originalValue\":\"Krimi\"") != std::string::npos);
    assert(resolvedJson.find("\"originalValue\":\"Crime\"") != std::string::npos);
    assert(resolvedJson.find("\"normalizedValue\":\"crime\"") != std::string::npos);
    assert(resolvedJson.find("\"confidence\":90") != std::string::npos);
    assert(resolvedJson.find("\"providerReference\":\"tmdb:80\"") != std::string::npos);

    GenreCollection escapedCollection = GenreCollection::createEmpty();

    escapedCollection.add(
        GenreClassification::from(
            ContentClassificationSource::User,
            "Krimi \"Nord\"",
            "crime-nord"));

    std::string escapedJson =
        serializer.serialize(
            resolver.resolve(escapedCollection));

    assert(escapedJson.find("Krimi \\\"Nord\\\"") != std::string::npos);

    std::string localizedGermanJson =
        serializer.serializeLocalized(
            resolved,
            "de_DE");

    assert(localizedGermanJson.find("\"label\":\"Krimi\"") != std::string::npos);
    assert(localizedGermanJson.find("\"locale\":\"de_DE\"") != std::string::npos);
    assert(localizedGermanJson.find("\"canonicalId\":\"crime\"") != std::string::npos);

    std::string localizedEnglishJson =
        serializer.serializeLocalized(
            resolved,
            "en_US");

    assert(localizedEnglishJson.find("\"label\":\"Crime\"") != std::string::npos);
    assert(localizedEnglishJson.find("\"locale\":\"en_US\"") != std::string::npos);

    std::cout << "test_genre_resolution_json_serializer passed" << std::endl;
    return 0;
}

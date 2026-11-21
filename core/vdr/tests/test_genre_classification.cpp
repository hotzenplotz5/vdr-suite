#include "GenreClassification.h"

#include <cassert>
#include <iostream>

int main()
{
    GenreClassification dvbGenre =
        GenreClassification::from(
            ContentClassificationSource::Dvb,
            "Spielfilm",
            "movie");

    assert(dvbGenre.source() == ContentClassificationSource::Dvb);
    assert(dvbGenre.originalValue() == "Spielfilm");
    assert(dvbGenre.normalizedValue() == "movie");
    assert(!dvbGenre.hasConfidence());
    assert(!dvbGenre.hasProviderReference());

    GenreClassification tmdbGenre =
        GenreClassification::withConfidence(
            ContentClassificationSource::Tmdb,
            "Crime",
            "crime",
            90);

    assert(tmdbGenre.source() == ContentClassificationSource::Tmdb);
    assert(tmdbGenre.originalValue() == "Crime");
    assert(tmdbGenre.normalizedValue() == "crime");
    assert(tmdbGenre.hasConfidence());
    assert(tmdbGenre.confidence() == 90);

    GenreClassification userGenre =
        GenreClassification::withProviderReference(
            ContentClassificationSource::User,
            "Lieblingsserie",
            "favorite-series",
            100,
            "profile:default");

    assert(userGenre.source() == ContentClassificationSource::User);
    assert(userGenre.hasProviderReference());
    assert(userGenre.providerReference() == "profile:default");

    GenreCollection collection = GenreCollection::createEmpty();

    assert(collection.empty());
    assert(collection.size() == 0);

    collection.add(dvbGenre);
    collection.add(tmdbGenre);
    collection.add(userGenre);

    assert(!collection.empty());
    assert(collection.size() == 3);
    assert(collection.hasSource(ContentClassificationSource::Dvb));
    assert(collection.hasSource(ContentClassificationSource::Tmdb));
    assert(collection.hasSource(ContentClassificationSource::User));
    assert(!collection.hasSource(ContentClassificationSource::Tvdb));

    assert(collection.hasNormalizedValue("movie"));
    assert(collection.hasNormalizedValue("crime"));
    assert(collection.hasNormalizedValue("favorite-series"));
    assert(!collection.hasNormalizedValue("comedy"));

    std::cout << "test_genre_classification passed" << std::endl;
    return 0;
}

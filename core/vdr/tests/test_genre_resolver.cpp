#include "GenreResolver.h"

#include <cassert>
#include <iostream>

int main()
{
    GenreResolver resolver;

    GenreCollection emptyCollection = GenreCollection::createEmpty();
    GenreResolutionResult unresolved = resolver.resolve(emptyCollection);

    assert(!unresolved.isResolved());
    assert(unresolved.evidenceCount() == 0);

    GenreCollection sourcePriorityCollection = GenreCollection::createEmpty();

    sourcePriorityCollection.add(
        GenreClassification::from(
            ContentClassificationSource::Dvb,
            "Spielfilm",
            "movie"));

    sourcePriorityCollection.add(
        GenreClassification::from(
            ContentClassificationSource::Tmdb,
            "Crime",
            "crime"));

    GenreResolutionResult sourcePriorityResult =
        resolver.resolve(sourcePriorityCollection);

    assert(sourcePriorityResult.isResolved());
    assert(sourcePriorityResult.primaryGenre().source() == ContentClassificationSource::Tmdb);
    assert(sourcePriorityResult.primaryGenre().normalizedValue() == "crime");
    assert(sourcePriorityResult.evidenceCount() == 2);
    assert(sourcePriorityResult.hasEvidenceFrom(ContentClassificationSource::Dvb));
    assert(sourcePriorityResult.hasEvidenceFrom(ContentClassificationSource::Tmdb));

    GenreCollection confidenceCollection = GenreCollection::createEmpty();

    confidenceCollection.add(
        GenreClassification::withConfidence(
            ContentClassificationSource::Tmdb,
            "Crime",
            "crime",
            60));

    confidenceCollection.add(
        GenreClassification::withConfidence(
            ContentClassificationSource::Dvb,
            "Krimi",
            "crime",
            90));

    GenreResolutionResult confidenceResult =
        resolver.resolve(confidenceCollection);

    assert(confidenceResult.isResolved());
    assert(confidenceResult.primaryGenre().source() == ContentClassificationSource::Dvb);
    assert(confidenceResult.primaryGenre().confidence() == 90);
    assert(confidenceResult.evidenceCount() == 2);

    GenreCollection userOverrideCollection = GenreCollection::createEmpty();

    userOverrideCollection.add(
        GenreClassification::withConfidence(
            ContentClassificationSource::Tmdb,
            "Crime",
            "crime",
            80));

    userOverrideCollection.add(
        GenreClassification::withConfidence(
            ContentClassificationSource::User,
            "Lieblingsserie",
            "favorite-series",
            80));

    GenreResolutionResult userOverrideResult =
        resolver.resolve(userOverrideCollection);

    assert(userOverrideResult.isResolved());
    assert(userOverrideResult.primaryGenre().source() == ContentClassificationSource::User);
    assert(userOverrideResult.primaryGenre().normalizedValue() == "favorite-series");
    assert(userOverrideResult.hasEvidenceFrom(ContentClassificationSource::Tmdb));
    assert(userOverrideResult.hasEvidenceFrom(ContentClassificationSource::User));

    std::cout << "test_genre_resolver passed" << std::endl;
    return 0;
}

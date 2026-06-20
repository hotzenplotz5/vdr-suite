#include "ContentRating.h"

#include <cassert>
#include <iostream>

int main()
{
    ContentRating fskRating =
        ContentRating::from(
            ContentClassificationSource::Dvb,
            ContentRatingSystem::Fsk,
            "FSK 12",
            12);

    assert(fskRating.source() == ContentClassificationSource::Dvb);
    assert(fskRating.system() == ContentRatingSystem::Fsk);
    assert(fskRating.originalValue() == "FSK 12");
    assert(fskRating.minimumAge() == 12);
    assert(!fskRating.hasConfidence());
    assert(!fskRating.hasProviderReference());

    ContentRating providerRating =
        ContentRating::withProviderReference(
            ContentClassificationSource::Tmdb,
            ContentRatingSystem::ProviderSpecific,
            "DE:16",
            16,
            90,
            "tmdb:release-certification:de");

    assert(providerRating.source() == ContentClassificationSource::Tmdb);
    assert(providerRating.minimumAge() == 16);
    assert(providerRating.hasConfidence());
    assert(providerRating.confidence() == 90);
    assert(providerRating.hasProviderReference());
    assert(providerRating.providerReference() == "tmdb:release-certification:de");

    ContentRatingCollection collection = ContentRatingCollection::createEmpty();

    assert(collection.empty());
    assert(collection.size() == 0);

    collection.add(fskRating);
    collection.add(providerRating);

    assert(!collection.empty());
    assert(collection.size() == 2);
    assert(collection.hasSystem(ContentRatingSystem::Fsk));
    assert(collection.hasSystem(ContentRatingSystem::ProviderSpecific));
    assert(!collection.hasSystem(ContentRatingSystem::Usk));
    assert(collection.hasMinimumAge(12));
    assert(collection.hasMinimumAge(16));
    assert(!collection.hasMinimumAge(18));

    std::cout << "test_content_rating passed" << std::endl;
    return 0;
}

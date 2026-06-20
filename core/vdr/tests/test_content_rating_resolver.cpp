#include "ContentRatingResolver.h"

#include <cassert>
#include <iostream>

int main()
{
    ContentRatingResolver resolver;

    ContentRatingResolutionResult unresolved =
        resolver.resolve(ContentRatingCollection::createEmpty());

    assert(!unresolved.isResolved());
    assert(unresolved.evidence().empty());

    ContentRatingCollection ratings = ContentRatingCollection::createEmpty();

    ratings.add(
        ContentRating::from(
            ContentClassificationSource::Dvb,
            ContentRatingSystem::Fsk,
            "FSK 12",
            12));

    ratings.add(
        ContentRating::withConfidence(
            ContentClassificationSource::Tmdb,
            ContentRatingSystem::ProviderSpecific,
            "DE:16",
            16,
            90));

    ContentRatingResolutionResult resolved = resolver.resolve(ratings);

    assert(resolved.isResolved());
    assert(resolved.primaryRating().source() == ContentClassificationSource::Tmdb);
    assert(resolved.primaryRating().minimumAge() == 16);
    assert(resolved.primaryRating().confidence() == 90);
    assert(resolved.evidence().size() == 2);

    ContentRatingCollection conservativeRatings = ContentRatingCollection::createEmpty();

    conservativeRatings.add(
        ContentRating::withConfidence(
            ContentClassificationSource::Tmdb,
            ContentRatingSystem::ProviderSpecific,
            "DE:12",
            12,
            80));

    conservativeRatings.add(
        ContentRating::withConfidence(
            ContentClassificationSource::Tvdb,
            ContentRatingSystem::ProviderSpecific,
            "DE:16",
            16,
            80));

    ContentRatingResolutionResult conservative =
        resolver.resolve(conservativeRatings);

    assert(conservative.isResolved());
    assert(conservative.primaryRating().minimumAge() == 16);

    ContentRatingCollection userOverrideRatings = ContentRatingCollection::createEmpty();

    userOverrideRatings.add(
        ContentRating::withConfidence(
            ContentClassificationSource::Tmdb,
            ContentRatingSystem::ProviderSpecific,
            "DE:16",
            16,
            80));

    userOverrideRatings.add(
        ContentRating::withConfidence(
            ContentClassificationSource::User,
            ContentRatingSystem::UserDefined,
            "Manual 12",
            12,
            80));

    ContentRatingResolutionResult userOverride =
        resolver.resolve(userOverrideRatings);

    assert(userOverride.isResolved());
    assert(userOverride.primaryRating().source() == ContentClassificationSource::User);
    assert(userOverride.primaryRating().minimumAge() == 12);

    std::cout << "test_content_rating_resolver passed" << std::endl;
    return 0;
}

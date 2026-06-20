#pragma once

#include "ContentRating.h"

class ContentRatingResolutionResult
{
public:
    static ContentRatingResolutionResult unresolved()
    {
        return ContentRatingResolutionResult();
    }

    static ContentRatingResolutionResult resolved(
        const ContentRating& primaryRating,
        const ContentRatingCollection& evidence)
    {
        ContentRatingResolutionResult result;
        result.resolved_ = true;
        result.primaryRating_ = primaryRating;
        result.evidence_ = evidence;
        return result;
    }

    bool isResolved() const
    {
        return resolved_;
    }

    const ContentRating& primaryRating() const
    {
        return primaryRating_;
    }

    const ContentRatingCollection& evidence() const
    {
        return evidence_;
    }

private:
    bool resolved_ = false;
    ContentRating primaryRating_ =
        ContentRating::from(
            ContentClassificationSource::Derived,
            ContentRatingSystem::Unknown,
            "",
            0);
    ContentRatingCollection evidence_ =
        ContentRatingCollection::createEmpty();
};

class ContentRatingResolver
{
public:
    ContentRatingResolutionResult resolve(
        const ContentRatingCollection& ratings) const
    {
        if (ratings.empty())
        {
            return ContentRatingResolutionResult::unresolved();
        }

        const ContentRating* selected = nullptr;

        for (const ContentRating& rating : ratings.all())
        {
            if (selected == nullptr || isBetter(rating, *selected))
            {
                selected = &rating;
            }
        }

        return ContentRatingResolutionResult::resolved(
            *selected,
            ratings);
    }

private:
    bool isBetter(
        const ContentRating& candidate,
        const ContentRating& current) const
    {
        if (candidate.hasConfidence() != current.hasConfidence())
        {
            return candidate.hasConfidence();
        }

        if (candidate.confidence() != current.confidence())
        {
            return candidate.confidence() > current.confidence();
        }

        if (candidate.source() == ContentClassificationSource::User &&
            current.source() != ContentClassificationSource::User)
        {
            return true;
        }

        if (candidate.source() != ContentClassificationSource::User &&
            current.source() == ContentClassificationSource::User)
        {
            return false;
        }

        if (candidate.minimumAge() != current.minimumAge())
        {
            return candidate.minimumAge() > current.minimumAge();
        }

        return sourcePriority(candidate.source()) > sourcePriority(current.source());
    }

    int sourcePriority(
        ContentClassificationSource source) const
    {
        switch (source)
        {
        case ContentClassificationSource::User:
            return 100;
        case ContentClassificationSource::Tmdb:
            return 90;
        case ContentClassificationSource::Tvdb:
            return 80;
        case ContentClassificationSource::Imdb:
            return 70;
        case ContentClassificationSource::Tvscraper:
            return 60;
        case ContentClassificationSource::Scraper2Vdr:
            return 50;
        case ContentClassificationSource::Dvb:
            return 40;
        case ContentClassificationSource::Epg:
            return 30;
        case ContentClassificationSource::Folder:
            return 20;
        case ContentClassificationSource::Derived:
            return 10;
        }

        return 0;
    }
};

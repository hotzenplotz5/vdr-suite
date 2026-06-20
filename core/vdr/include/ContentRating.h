#pragma once

#include "GenreClassification.h"

#include <string>
#include <vector>

enum class ContentRatingSystem
{
    Unknown,
    Fsk,
    Usk,
    TvParentalGuideline,
    ProviderSpecific,
    UserDefined
};

class ContentRating
{
public:
    static ContentRating from(
        ContentClassificationSource source,
        ContentRatingSystem system,
        const std::string& originalValue,
        int minimumAge)
    {
        return ContentRating(
            source,
            system,
            originalValue,
            minimumAge,
            0,
            "");
    }

    static ContentRating withConfidence(
        ContentClassificationSource source,
        ContentRatingSystem system,
        const std::string& originalValue,
        int minimumAge,
        int confidence)
    {
        return ContentRating(
            source,
            system,
            originalValue,
            minimumAge,
            confidence,
            "");
    }

    static ContentRating withProviderReference(
        ContentClassificationSource source,
        ContentRatingSystem system,
        const std::string& originalValue,
        int minimumAge,
        int confidence,
        const std::string& providerReference)
    {
        return ContentRating(
            source,
            system,
            originalValue,
            minimumAge,
            confidence,
            providerReference);
    }

    ContentClassificationSource source() const
    {
        return source_;
    }

    ContentRatingSystem system() const
    {
        return system_;
    }

    const std::string& originalValue() const
    {
        return originalValue_;
    }

    int minimumAge() const
    {
        return minimumAge_;
    }

    int confidence() const
    {
        return confidence_;
    }

    bool hasConfidence() const
    {
        return confidence_ > 0;
    }

    const std::string& providerReference() const
    {
        return providerReference_;
    }

    bool hasProviderReference() const
    {
        return !providerReference_.empty();
    }

private:
    ContentRating(
        ContentClassificationSource source,
        ContentRatingSystem system,
        const std::string& originalValue,
        int minimumAge,
        int confidence,
        const std::string& providerReference)
        : source_(source),
          system_(system),
          originalValue_(originalValue),
          minimumAge_(minimumAge),
          confidence_(confidence),
          providerReference_(providerReference)
    {
    }

    ContentClassificationSource source_;
    ContentRatingSystem system_;
    std::string originalValue_;
    int minimumAge_ = 0;
    int confidence_ = 0;
    std::string providerReference_;
};

class ContentRatingCollection
{
public:
    static ContentRatingCollection createEmpty()
    {
        return ContentRatingCollection();
    }

    void add(
        const ContentRating& rating)
    {
        ratings_.push_back(rating);
    }

    const std::vector<ContentRating>& all() const
    {
        return ratings_;
    }

    bool empty() const
    {
        return ratings_.empty();
    }

    int size() const
    {
        return static_cast<int>(ratings_.size());
    }

    bool hasSystem(
        ContentRatingSystem system) const
    {
        for (const ContentRating& rating : ratings_)
        {
            if (rating.system() == system)
            {
                return true;
            }
        }

        return false;
    }

    bool hasMinimumAge(
        int minimumAge) const
    {
        for (const ContentRating& rating : ratings_)
        {
            if (rating.minimumAge() == minimumAge)
            {
                return true;
            }
        }

        return false;
    }

private:
    std::vector<ContentRating> ratings_;
};

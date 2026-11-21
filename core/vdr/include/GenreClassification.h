#pragma once

#include <string>
#include <vector>

enum class ContentClassificationSource
{
    Epg,
    Dvb,
    Tvscraper,
    Scraper2Vdr,
    Tmdb,
    Tvdb,
    Imdb,
    User,
    Folder,
    Derived
};

class GenreClassification
{
public:
    static GenreClassification from(
        ContentClassificationSource source,
        const std::string& originalValue,
        const std::string& normalizedValue)
    {
        return GenreClassification(
            source,
            originalValue,
            normalizedValue,
            0,
            "");
    }

    static GenreClassification withConfidence(
        ContentClassificationSource source,
        const std::string& originalValue,
        const std::string& normalizedValue,
        int confidence)
    {
        return GenreClassification(
            source,
            originalValue,
            normalizedValue,
            confidence,
            "");
    }

    static GenreClassification withProviderReference(
        ContentClassificationSource source,
        const std::string& originalValue,
        const std::string& normalizedValue,
        int confidence,
        const std::string& providerReference)
    {
        return GenreClassification(
            source,
            originalValue,
            normalizedValue,
            confidence,
            providerReference);
    }

    ContentClassificationSource source() const
    {
        return source_;
    }

    const std::string& originalValue() const
    {
        return originalValue_;
    }

    const std::string& normalizedValue() const
    {
        return normalizedValue_;
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
    GenreClassification(
        ContentClassificationSource source,
        const std::string& originalValue,
        const std::string& normalizedValue,
        int confidence,
        const std::string& providerReference)
        : source_(source),
          originalValue_(originalValue),
          normalizedValue_(normalizedValue),
          confidence_(confidence),
          providerReference_(providerReference)
    {
    }

    ContentClassificationSource source_;
    std::string originalValue_;
    std::string normalizedValue_;
    int confidence_ = 0;
    std::string providerReference_;
};

class GenreCollection
{
public:
    static GenreCollection createEmpty()
    {
        return GenreCollection();
    }

    void add(
        const GenreClassification& genre)
    {
        genres_.push_back(genre);
    }

    const std::vector<GenreClassification>& all() const
    {
        return genres_;
    }

    bool empty() const
    {
        return genres_.empty();
    }

    int size() const
    {
        return static_cast<int>(genres_.size());
    }

    bool hasSource(
        ContentClassificationSource source) const
    {
        for (const GenreClassification& genre : genres_)
        {
            if (genre.source() == source)
            {
                return true;
            }
        }

        return false;
    }

    bool hasNormalizedValue(
        const std::string& normalizedValue) const
    {
        for (const GenreClassification& genre : genres_)
        {
            if (genre.normalizedValue() == normalizedValue)
            {
                return true;
            }
        }

        return false;
    }

private:
    std::vector<GenreClassification> genres_;
};

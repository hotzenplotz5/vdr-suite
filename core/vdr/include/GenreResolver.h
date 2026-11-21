#pragma once

#include "GenreClassification.h"

#include <string>
#include <vector>

class GenreResolutionResult
{
public:
    static GenreResolutionResult unresolved()
    {
        return GenreResolutionResult();
    }

    static GenreResolutionResult resolved(
        const GenreClassification& primaryGenre,
        const std::vector<GenreClassification>& evidence)
    {
        GenreResolutionResult result;

        result.resolved_ = true;
        result.primaryGenre_ = primaryGenre;
        result.evidence_ = evidence;

        return result;
    }

    bool isResolved() const
    {
        return resolved_;
    }

    const GenreClassification& primaryGenre() const
    {
        return primaryGenre_;
    }

    const std::vector<GenreClassification>& evidence() const
    {
        return evidence_;
    }

    int evidenceCount() const
    {
        return static_cast<int>(evidence_.size());
    }

    bool hasEvidenceFrom(
        ContentClassificationSource source) const
    {
        for (const GenreClassification& genre : evidence_)
        {
            if (genre.source() == source)
            {
                return true;
            }
        }

        return false;
    }

private:
    bool resolved_ = false;
    GenreClassification primaryGenre_ =
        GenreClassification::from(
            ContentClassificationSource::Derived,
            "",
            "");
    std::vector<GenreClassification> evidence_;
};

class GenreResolver
{
public:
    GenreResolutionResult resolve(
        const GenreCollection& genres) const
    {
        if (genres.empty())
        {
            return GenreResolutionResult::unresolved();
        }

        const std::vector<GenreClassification>& evidence = genres.all();
        const GenreClassification* best = &evidence.front();

        for (const GenreClassification& candidate : evidence)
        {
            if (isBetter(
                    candidate,
                    *best))
            {
                best = &candidate;
            }
        }

        return GenreResolutionResult::resolved(
            *best,
            evidence);
    }

private:
    bool isBetter(
        const GenreClassification& candidate,
        const GenreClassification& current) const
    {
        if (candidate.hasConfidence() != current.hasConfidence())
        {
            return candidate.hasConfidence();
        }

        if (candidate.confidence() != current.confidence())
        {
            return candidate.confidence() > current.confidence();
        }

        return sourcePriority(candidate.source()) >
               sourcePriority(current.source());
    }

    int sourcePriority(
        ContentClassificationSource source) const
    {
        switch (source)
        {
        case ContentClassificationSource::User:
            return 100;
        case ContentClassificationSource::Folder:
            return 90;
        case ContentClassificationSource::Tmdb:
            return 80;
        case ContentClassificationSource::Tvdb:
            return 75;
        case ContentClassificationSource::Imdb:
            return 70;
        case ContentClassificationSource::Tvscraper:
            return 60;
        case ContentClassificationSource::Scraper2Vdr:
            return 55;
        case ContentClassificationSource::Dvb:
            return 40;
        case ContentClassificationSource::Epg:
            return 30;
        case ContentClassificationSource::Derived:
            return 20;
        }

        return 0;
    }
};

#pragma once

#include <string>

enum class SeriesArtworkProviderCacheOutcome
{
    None,
    NotFound,
    TemporarilyUnavailable
};

struct SeriesArtworkProviderCacheKey
{
    std::string provider;
    std::string identityProvider;
    std::string identityValue;

    bool valid() const
    {
        return !provider.empty() && !identityProvider.empty() &&
            !identityValue.empty();
    }
};

struct SeriesArtworkProviderCacheEntry
{
    SeriesArtworkProviderCacheOutcome outcome =
        SeriesArtworkProviderCacheOutcome::None;
    long long expiresAt = 0;

    bool active(long long now) const
    {
        return outcome != SeriesArtworkProviderCacheOutcome::None &&
            expiresAt > now;
    }
};

class ISeriesArtworkProviderCache
{
public:
    virtual ~ISeriesArtworkProviderCache() = default;

    virtual SeriesArtworkProviderCacheEntry find(
        const SeriesArtworkProviderCacheKey& key,
        long long now) = 0;

    virtual bool store(
        const SeriesArtworkProviderCacheKey& key,
        SeriesArtworkProviderCacheOutcome outcome,
        long long expiresAt) = 0;

    virtual bool remove(
        const SeriesArtworkProviderCacheKey& key) = 0;
};

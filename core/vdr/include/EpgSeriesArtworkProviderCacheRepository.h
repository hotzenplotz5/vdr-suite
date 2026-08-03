#pragma once

#include "ISeriesArtworkProviderCache.h"

#include <mutex>

class Database;

class EpgSeriesArtworkProviderCacheRepository final
    : public ISeriesArtworkProviderCache
{
public:
    explicit EpgSeriesArtworkProviderCacheRepository(Database& database);

    bool ensureSchema();

    SeriesArtworkProviderCacheEntry find(
        const SeriesArtworkProviderCacheKey& key,
        long long now) override;

    bool store(
        const SeriesArtworkProviderCacheKey& key,
        SeriesArtworkProviderCacheOutcome outcome,
        long long expiresAt) override;

    bool remove(
        const SeriesArtworkProviderCacheKey& key) override;

private:
    Database& database_;
    std::mutex mutex_;

    bool ensureSchemaLocked();
};

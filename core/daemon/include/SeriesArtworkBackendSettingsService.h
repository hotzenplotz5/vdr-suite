#pragma once

#include "Database.h"
#include "IExternalArtworkHttpTransport.h"
#include "ISeriesArtworkFallbackProvider.h"
#include "ISeriesArtworkProviderCache.h"
#include "TmdbSeriesArtworkProvider.h"
#include "TvmazeSeriesArtworkProvider.h"

#include <mutex>
#include <string>

struct SeriesArtworkBackendSettingsConfig
{
    std::string defaultProvider = "none";
    std::string environmentTmdbReadAccessToken;
    std::string secretRoot =
        "/var/lib/vdr-suite/secrets/series-artwork";
    TmdbSeriesArtworkProviderConfig tmdb;
    TvmazeSeriesArtworkProviderConfig tvmaze;
};

struct SeriesArtworkBackendSettingsSnapshot
{
    std::string backendId;
    std::string provider = "none";
    std::string configurationSource = "environment";
    bool tmdbTokenConfigured = false;
    std::string tmdbTokenSource = "none";
};

struct SeriesArtworkBackendSettingsUpdate
{
    std::string backendId;
    std::string provider;
    std::string tmdbReadAccessToken;
    bool clearTmdbReadAccessToken = false;
};

struct SeriesArtworkBackendSettingsUpdateResult
{
    bool success = false;
    int statusCode = 500;
    std::string errorCode;
    std::string message;
    SeriesArtworkBackendSettingsSnapshot settings;
};

class SeriesArtworkBackendSettingsService final
    : public ISeriesArtworkFallbackProvider
{
public:
    SeriesArtworkBackendSettingsService(
        Database& database,
        IExternalArtworkHttpTransport& transport,
        ISeriesArtworkProviderCache& cache,
        SeriesArtworkBackendSettingsConfig config);

    bool ensureSchema();

    SeriesArtworkBackendSettingsSnapshot get(
        const std::string& backendId) const;

    SeriesArtworkBackendSettingsUpdateResult update(
        const SeriesArtworkBackendSettingsUpdate& request);

    SeriesArtworkFallbackResolution resolve(
        const std::string& backendId,
        const VdrEvent& event,
        const EpgScraperMetadata& metadata) override;

    static bool validBackendId(const std::string& backendId);
    static bool validProvider(const std::string& provider);

private:
    enum class TokenValidation
    {
        Valid,
        Invalid,
        Unavailable
    };

    bool ensureSchemaLocked() const;
    bool readManagedProviderLocked(
        const std::string& backendId,
        std::string& provider) const;
    bool storeManagedProviderLocked(
        const std::string& backendId,
        const std::string& provider) const;

    std::string readManagedTokenLocked(
        const std::string& backendId) const;
    bool writeManagedTokenLocked(
        const std::string& backendId,
        const std::string& token) const;
    bool removeManagedTokenLocked(
        const std::string& backendId) const;

    SeriesArtworkBackendSettingsSnapshot snapshotLocked(
        const std::string& backendId,
        std::string* effectiveToken = nullptr) const;

    TokenValidation validateTmdbToken(
        const std::string& token) const;

    Database& database_;
    IExternalArtworkHttpTransport& transport_;
    ISeriesArtworkProviderCache& cache_;
    SeriesArtworkBackendSettingsConfig config_;
    mutable std::mutex mutex_;
};

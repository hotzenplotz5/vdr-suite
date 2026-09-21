#pragma once

#include "Database.h"
#include "IExternalArtworkHttpTransport.h"
#include "ISeriesArtworkFallbackProvider.h"
#include "ISeriesArtworkProviderCache.h"
#include "TmdbSeriesArtworkProvider.h"
#include "TvmazeSeriesArtworkProvider.h"

#include <mutex>
#include <string>
#include <vector>

struct SeriesArtworkBackendSettingsConfig
{
    std::string defaultProvider = "none";
    std::string environmentTmdbReadAccessToken;
    std::string secretRoot =
        "/var/lib/vdr-suite/secrets/series-artwork";
    std::string seriesCoverCacheRoot =
        "/var/cache/vdr-suite/series-covers";
    TmdbSeriesArtworkProviderConfig tmdb;
    TvmazeSeriesArtworkProviderConfig tvmaze;
};

struct SeriesArtworkCoverOverride
{
    std::string seriesKey;
    std::string posterUrl;
    std::string providerId;
    std::string externalNamespace;
    std::string externalId;
    std::string posterReference;
    int revision = 0;
};

struct SeriesArtworkBackendSettingsSnapshot
{
    std::string backendId;
    std::string provider = "none";
    std::string configurationSource = "environment";
    bool tmdbTokenConfigured = false;
    std::string tmdbTokenSource = "none";
    std::vector<SeriesArtworkCoverOverride> coverOverrides;
};

struct SeriesArtworkBackendSettingsUpdate
{
    std::string backendId;
    std::string provider;
    std::string tmdbReadAccessToken;
    bool clearTmdbReadAccessToken = false;
    std::string operation;
    std::string seriesKey;
    std::string posterUrl;
    std::string providerId;
    std::string externalNamespace;
    std::string externalId;
    std::string posterReference;
};

struct SeriesArtworkImageResult
{
    bool success = false;
    int statusCode = 404;
    std::string errorCode;
    std::string message;
    std::string contentType;
    std::string body;
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

    SeriesArtworkImageResult coverImage(
        const std::string& backendId,
        const std::string& seriesKey) const;

    SeriesArtworkImageResult tmdbCandidateImage(
        const std::string& backendId,
        const std::string& externalId,
        const std::string& posterReference) const;

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

    std::vector<SeriesArtworkCoverOverride>
    loadCoverOverridesLocked(
        const std::string& backendId) const;

    bool storeCoverOverrideLocked(
        const std::string& backendId,
        const std::string& seriesKey,
        const std::string& posterUrl) const;

    bool storeTmdbCoverOverrideLocked(
        const std::string& backendId,
        const std::string& seriesKey,
        const std::string& posterPath,
        const std::string& externalId,
        const std::string& posterReference) const;

    std::string materializeTmdbSeriesPoster(
        const std::string& backendId,
        const std::string& externalId,
        const std::string& posterReference) const;

    bool removeCoverOverrideLocked(
        const std::string& backendId,
        const std::string& seriesKey) const;

    static bool validSeriesKey(const std::string& seriesKey);
    static bool validCoverPosterUrl(const std::string& posterUrl);

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

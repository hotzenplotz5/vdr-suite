#pragma once

#include "EpgArtworkPathPolicy.h"
#include "EpgScraperMetadataPublicJsonSerializer.h"
#include "IEpgScraperMetadataResolver.h"
#include "IEpgSeriesArtworkFallbackDeliveryProvider.h"

#include <string>
#include <vector>

class EpgArtworkRepository;

class PersistentEpgScraperMetadataResolver final
    : public IEpgScraperMetadataResolver,
      public IEpgSeriesArtworkFallbackDeliveryProvider
{
public:
    PersistentEpgScraperMetadataResolver(
        IEpgScraperMetadataResolver& delegate,
        EpgArtworkRepository& artworkRepository,
        std::vector<std::string> allowedRoots =
            EpgArtworkPathPolicy::defaultAllowedRoots());

    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

    EpgSeriesArtworkFallbackAsset loadSeriesArtworkFallback(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const override;

private:
    IEpgScraperMetadataResolver& delegate_;
    IEpgSeriesArtworkFallbackDeliveryProvider* fallbackDeliveryProvider_;
    EpgArtworkRepository& artworkRepository_;
    std::vector<std::string> allowedRoots_;
    EpgScraperMetadataPublicJsonSerializer serializer_;
};

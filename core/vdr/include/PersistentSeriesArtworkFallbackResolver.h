#pragma once

#include "EpgArtworkPathPolicy.h"
#include "EpgSeriesArtworkFallbackDeliveryService.h"
#include "IEpgScraperMetadataResolver.h"

#include <string>
#include <vector>

class EpgSeriesArtworkFallbackRepository;

class PersistentSeriesArtworkFallbackResolver final
    : public IEpgScraperMetadataResolver,
      public IEpgSeriesArtworkFallbackDeliveryProvider
{
public:
    PersistentSeriesArtworkFallbackResolver(
        IEpgScraperMetadataResolver& delegate,
        EpgSeriesArtworkFallbackRepository& repository,
        std::vector<std::string> allowedRoots);

    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

    EpgSeriesArtworkFallbackAsset loadSeriesArtworkFallback(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const override;

private:
    IEpgScraperMetadataResolver& delegate_;
    EpgSeriesArtworkFallbackRepository& repository_;
    std::vector<std::string> allowedRoots_;
    EpgSeriesArtworkFallbackDeliveryService deliveryService_;
};

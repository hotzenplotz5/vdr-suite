#pragma once

#include "IEpgSeriesArtworkFallbackDeliveryProvider.h"

#include <cstdint>
#include <string>
#include <vector>

class EpgSeriesArtworkFallbackRepository;

struct EpgSeriesArtworkFallbackDeliveryConfig
{
    std::vector<std::string> managedRoots;
    std::uintmax_t maximumBytes = 16U * 1024U * 1024U;
    int maximumDimension = 16384;
    std::uint64_t maximumPixels = 100000000U;
};

class EpgSeriesArtworkFallbackDeliveryService final
    : public IEpgSeriesArtworkFallbackDeliveryProvider
{
public:
    EpgSeriesArtworkFallbackDeliveryService(
        EpgSeriesArtworkFallbackRepository& repository,
        EpgSeriesArtworkFallbackDeliveryConfig config);

    EpgSeriesArtworkFallbackAsset loadSeriesArtworkFallback(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const override;

private:
    EpgSeriesArtworkFallbackRepository& repository_;
    EpgSeriesArtworkFallbackDeliveryConfig config_;
};

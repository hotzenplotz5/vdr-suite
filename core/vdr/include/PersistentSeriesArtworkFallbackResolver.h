#pragma once

#include "EpgArtworkPathPolicy.h"
#include "IEpgScraperMetadataResolver.h"

#include <string>
#include <vector>

class EpgSeriesArtworkFallbackRepository;

class PersistentSeriesArtworkFallbackResolver final
    : public IEpgScraperMetadataResolver
{
public:
    PersistentSeriesArtworkFallbackResolver(
        IEpgScraperMetadataResolver& delegate,
        EpgSeriesArtworkFallbackRepository& repository,
        std::vector<std::string> allowedRoots);

    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

private:
    IEpgScraperMetadataResolver& delegate_;
    EpgSeriesArtworkFallbackRepository& repository_;
    std::vector<std::string> allowedRoots_;
};

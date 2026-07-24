#pragma once

#include "EpgArtworkPathPolicy.h"
#include "EpgScraperMetadataPublicJsonSerializer.h"
#include "IEpgScraperMetadataResolver.h"

#include <string>
#include <vector>

class EpgArtworkRepository;

class PersistentEpgScraperMetadataResolver final
    : public IEpgScraperMetadataResolver
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

private:
    IEpgScraperMetadataResolver& delegate_;
    EpgArtworkRepository& artworkRepository_;
    std::vector<std::string> allowedRoots_;
    EpgScraperMetadataPublicJsonSerializer serializer_;
};

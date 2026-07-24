#pragma once

#include "IEpgScraperMetadataResolver.h"

class EpgArtworkRepository;

class PersistentEpgScraperMetadataResolver final
    : public IEpgScraperMetadataResolver
{
public:
    PersistentEpgScraperMetadataResolver(
        IEpgScraperMetadataResolver& delegate,
        EpgArtworkRepository& artworkRepository);

    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

private:
    IEpgScraperMetadataResolver& delegate_;
    EpgArtworkRepository& artworkRepository_;
};

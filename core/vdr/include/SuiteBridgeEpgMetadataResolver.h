#pragma once

#include "IEpgScraperMetadataResolver.h"
#include "ISuiteBridgeMetadataTransport.h"

class SuiteBridgeEpgMetadataResolver final : public IEpgScraperMetadataResolver
{
public:
    explicit SuiteBridgeEpgMetadataResolver(
        ISuiteBridgeMetadataTransport& transport);

    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

private:
    ISuiteBridgeMetadataTransport& transport_;
};

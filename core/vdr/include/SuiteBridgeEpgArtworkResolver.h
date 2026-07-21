#pragma once

#include "IEpgArtworkResolver.h"
#include "IEpgScraperMetadataResolverProvider.h"
#include "ISuiteBridgeArtworkTransport.h"
#include "ISuiteBridgeMetadataTransport.h"
#include "SuiteBridgeEpgMetadataResolver.h"

#include <memory>
#include <mutex>

class SuiteBridgeEpgArtworkResolver final :
    public IEpgArtworkResolver,
    public IEpgScraperMetadataResolverProvider
{
public:
    explicit SuiteBridgeEpgArtworkResolver(ISuiteBridgeArtworkTransport& transport);

    EpgArtworkResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

    IEpgScraperMetadataResolver* scraperMetadataResolver() override
    {
        std::lock_guard<std::mutex> lock(metadataResolverMutex_);
        if (!metadataResolver_)
        {
            auto* metadataTransport =
                dynamic_cast<ISuiteBridgeMetadataTransport*>(&transport_);
            if (metadataTransport == nullptr)
            {
                return nullptr;
            }

            metadataResolver_ =
                std::make_unique<SuiteBridgeEpgMetadataResolver>(
                    *metadataTransport);
        }

        return metadataResolver_.get();
    }

private:
    ISuiteBridgeArtworkTransport& transport_;
    std::mutex metadataResolverMutex_;
    std::unique_ptr<SuiteBridgeEpgMetadataResolver> metadataResolver_;
};

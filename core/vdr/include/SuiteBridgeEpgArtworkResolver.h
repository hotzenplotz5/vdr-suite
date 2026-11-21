#pragma once

#include "IEpgArtworkResolver.h"
#include "ISuiteBridgeArtworkTransport.h"

class SuiteBridgeEpgArtworkResolver final : public IEpgArtworkResolver
{
public:
    explicit SuiteBridgeEpgArtworkResolver(ISuiteBridgeArtworkTransport& transport);

    EpgArtworkResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

private:
    ISuiteBridgeArtworkTransport& transport_;
};

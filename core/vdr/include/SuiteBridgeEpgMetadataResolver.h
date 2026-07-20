#pragma once

#include "EpgMetadataJsonParser.h"
#include "IEpgMetadataResolver.h"
#include "ISuiteBridgeEpgMetadataTransport.h"

class SuiteBridgeEpgMetadataResolver final : public IEpgMetadataResolver
{
public:
    explicit SuiteBridgeEpgMetadataResolver(
        ISuiteBridgeEpgMetadataTransport& transport);

    EpgMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override;

private:
    ISuiteBridgeEpgMetadataTransport& transport_;
    EpgMetadataJsonParser parser_;
};

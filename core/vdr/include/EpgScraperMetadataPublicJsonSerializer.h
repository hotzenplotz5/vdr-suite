#pragma once

#include "EpgScraperMetadata.h"

#include <string>

class EpgScraperMetadataPublicJsonSerializer
{
public:
    std::string serialize(
        const EpgScraperMetadataResolution& resolution) const;

private:
    static std::string imageUrl(
        const EpgScraperMetadata& metadata,
        const std::string& kind,
        int index);
};

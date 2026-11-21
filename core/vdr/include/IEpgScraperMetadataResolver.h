#pragma once

#include "EpgScraperMetadata.h"
#include "VdrEvent.h"

#include <string>

class IEpgScraperMetadataResolver
{
public:
    virtual ~IEpgScraperMetadataResolver() = default;

    virtual EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) = 0;
};

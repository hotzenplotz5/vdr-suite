#pragma once

#include "EpgMetadataRecord.h"
#include "VdrEvent.h"

#include <string>

struct EpgMetadataResolution
{
    bool attempted = false;
    bool found = false;
    EpgMetadataRecord metadata;
};

class IEpgMetadataResolver
{
public:
    virtual ~IEpgMetadataResolver() = default;

    virtual EpgMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) = 0;
};

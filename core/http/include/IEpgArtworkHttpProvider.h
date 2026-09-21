#pragma once

#include "HttpServerResponse.h"

#include <string>

class IEpgArtworkHttpProvider
{
public:
    virtual ~IEpgArtworkHttpProvider() = default;

    virtual HttpServerResponse getEpgArtwork(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const = 0;
};

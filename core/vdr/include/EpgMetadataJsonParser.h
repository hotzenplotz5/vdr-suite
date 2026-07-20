#pragma once

#include "EpgMetadataRecord.h"

#include <string>

class EpgMetadataJsonParser
{
public:
    EpgMetadataRecord parse(
        const std::string& payload,
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId,
        long long resolvedAt) const;
};

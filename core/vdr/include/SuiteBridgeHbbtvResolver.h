#pragma once

#include "HbbtvDomain.h"
#include "ISuiteBridgeHbbtvTransport.h"

#include <cstdint>
#include <string>

class SuiteBridgeHbbtvResolver
{
public:
    explicit SuiteBridgeHbbtvResolver(
        ISuiteBridgeHbbtvTransport& transport);

    BroadcastApplicationDiscoverySnapshot discoverApplications(
        const std::string& backendId,
        std::uint64_t backendGeneration,
        const std::string& channelId) const;

private:
    ISuiteBridgeHbbtvTransport& transport_;
};

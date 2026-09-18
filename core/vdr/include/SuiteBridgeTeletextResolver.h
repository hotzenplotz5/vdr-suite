#pragma once

#include "ISuiteBridgeTeletextTransport.h"
#include "TeletextDomain.h"

#include <cstdint>
#include <string>

class SuiteBridgeTeletextResolver
{
public:
    explicit SuiteBridgeTeletextResolver(
        ISuiteBridgeTeletextTransport& transport);

    TeletextServiceSnapshot discoverService(
        const std::string& backendId,
        std::uint64_t backendGeneration,
        const std::string& channelId) const;

    TeletextPageSnapshot readPage(
        const TeletextServiceRef& service,
        std::uint16_t pageNumber,
        bool automaticSubpage = true,
        std::uint16_t subpageCode = 0) const;

private:
    ISuiteBridgeTeletextTransport& transport_;
};

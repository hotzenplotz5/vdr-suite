#pragma once

#include "ISuiteBridgeHbbtvTransport.h"

#include <cstdint>
#include <string>

struct HbbtvPresentationFrame
{
    bool available = false;
    bool unchanged = false;
    std::string error;
    std::uint64_t frameRevision = 0;
    std::uint64_t observedAt = 0;
    std::uint32_t renderWidth = 0;
    std::uint32_t renderHeight = 0;
    std::string qoi;
};

class IHbbtvPresentationSource
{
public:
    virtual ~IHbbtvPresentationSource() = default;

    virtual HbbtvPresentationFrame readPresentation(
        const std::string& sessionId,
        std::uint64_t knownRevision) = 0;
};

class SuiteBridgeHbbtvPresentationResolver final :
    public IHbbtvPresentationSource
{
public:
    explicit SuiteBridgeHbbtvPresentationResolver(
        ISuiteBridgeHbbtvTransport& transport);

    HbbtvPresentationFrame readPresentation(
        const std::string& sessionId,
        std::uint64_t knownRevision) override;

private:
    ISuiteBridgeHbbtvTransport& transport_;
};

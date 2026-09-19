#pragma once

#include "ISuiteBridgeHbbtvTransport.h"

#include <cstdint>
#include <string>

enum class HbbtvMediaSourceState
{
    None,
    Streaming,
    Paused,
    Stopped,
    Failed
};

struct HbbtvMediaSource
{
    bool available = false;
    std::string error;
    HbbtvMediaSourceState state = HbbtvMediaSourceState::None;
    std::uint64_t mediaRevision = 0;
    bool fullscreen = true;
    bool consumerConnected = false;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    // Private provider topology. Never serialize this field to a public API.
    std::string unixSocketPath;
};

class IHbbtvMediaSourceResolver
{
public:
    virtual ~IHbbtvMediaSourceResolver() = default;
    virtual HbbtvMediaSource resolveMedia(const std::string& sessionId) = 0;
};

class SuiteBridgeHbbtvMediaResolver final : public IHbbtvMediaSourceResolver
{
public:
    explicit SuiteBridgeHbbtvMediaResolver(ISuiteBridgeHbbtvTransport& transport);
    HbbtvMediaSource resolveMedia(const std::string& sessionId) override;

private:
    ISuiteBridgeHbbtvTransport& transport_;
};

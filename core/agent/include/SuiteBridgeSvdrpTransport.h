#pragma once

#include "ISuiteBridgeLocalTransport.h"
#include "ISuiteBridgeArtworkTransport.h"
#include "ISuiteBridgeMetadataTransport.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

struct SuiteBridgeSvdrpTransportConfig
{
    std::string host = "127.0.0.1";
    int port = 6419;
    std::chrono::milliseconds connectTimeout{1000};
    std::chrono::milliseconds ioTimeout{1000};
    std::chrono::milliseconds operationTimeout{3000};
};

class SuiteBridgeSvdrpTransport final :
    public ISuiteBridgeLocalTransport,
    public ::ISuiteBridgeArtworkTransport,
    public ::ISuiteBridgeMetadataTransport
{
public:
    static constexpr std::size_t MaximumGreetingBytes = 1024;
    static constexpr std::size_t MaximumReplyBytes = 8192;
    static constexpr std::size_t MaximumReplyLines = 64;

    explicit SuiteBridgeSvdrpTransport(
        SuiteBridgeSvdrpTransportConfig config = {});

    SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand command) override;

    ::SuiteBridgeArtworkCommandReply requestArtwork(
        const std::string& channelId,
        const std::string& eventId) override;

    ::SuiteBridgeMetadataCommandReply requestMetadata(
        const std::string& channelId,
        const std::string& eventId) override;

private:
    SuiteBridgeCommandReply executeRequest(
        const std::string& requestText);

    SuiteBridgeSvdrpTransportConfig config_;
};

}

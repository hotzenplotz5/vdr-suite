#pragma once

#include "ISuiteBridgeLocalTransport.h"

#include <chrono>
#include <cstddef>
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

class SuiteBridgeSvdrpTransport final : public ISuiteBridgeLocalTransport
{
public:
    static constexpr std::size_t MaximumGreetingBytes = 1024;
    static constexpr std::size_t MaximumReplyBytes = 8192;
    static constexpr std::size_t MaximumReplyLines = 64;

    explicit SuiteBridgeSvdrpTransport(
        SuiteBridgeSvdrpTransportConfig config = {});

    SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand command) override;

private:
    SuiteBridgeSvdrpTransportConfig config_;
};

}
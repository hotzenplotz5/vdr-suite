#pragma once

#include <string>

struct SuiteBridgeHbbtvCommandReply
{
    bool transportSucceeded = false;
    int replyCode = 0;
    std::string payload;
};

class ISuiteBridgeHbbtvTransport
{
public:
    virtual ~ISuiteBridgeHbbtvTransport() = default;

    virtual SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string& channelId) = 0;
};

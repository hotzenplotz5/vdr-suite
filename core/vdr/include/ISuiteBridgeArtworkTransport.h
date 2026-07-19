#pragma once

#include <string>

struct SuiteBridgeArtworkCommandReply
{
    bool transportSucceeded = false;
    int replyCode = 0;
    std::string payload;
};

class ISuiteBridgeArtworkTransport
{
public:
    virtual ~ISuiteBridgeArtworkTransport() = default;

    virtual SuiteBridgeArtworkCommandReply requestArtwork(
        const std::string& channelId,
        const std::string& eventId) = 0;
};

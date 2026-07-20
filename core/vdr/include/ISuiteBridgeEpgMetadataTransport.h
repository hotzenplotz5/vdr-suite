#pragma once

#include <string>

struct SuiteBridgeEpgMetadataCommandReply
{
    bool transportSucceeded = false;
    int replyCode = 0;
    std::string payload;
};

class ISuiteBridgeEpgMetadataTransport
{
public:
    virtual ~ISuiteBridgeEpgMetadataTransport() = default;

    virtual SuiteBridgeEpgMetadataCommandReply requestEpgMetadata(
        const std::string& channelId,
        const std::string& eventId) = 0;
};

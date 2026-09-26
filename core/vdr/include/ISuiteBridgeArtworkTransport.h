#pragma once

#include "SuiteBridgeReadTransportStatus.h"

#include <string>

struct SuiteBridgeArtworkCommandReply
{
    bool transportSucceeded = false;
    SuiteBridgeReadTransportStatus transportStatus =
        SuiteBridgeReadTransportStatus::Failed;
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

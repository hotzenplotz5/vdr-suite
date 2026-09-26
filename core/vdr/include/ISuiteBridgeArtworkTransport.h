#pragma once

#include "SuiteBridgeReadTransportStatus.h"

#include <string>

struct SuiteBridgeArtworkCommandReply
{
    bool transportSucceeded = false;
    int replyCode = 0;
    std::string payload;
    SuiteBridgeReadTransportStatus transportStatus =
        SuiteBridgeReadTransportStatus::Failed;
};

class ISuiteBridgeArtworkTransport
{
public:
    virtual ~ISuiteBridgeArtworkTransport() = default;

    virtual SuiteBridgeArtworkCommandReply requestArtwork(
        const std::string& channelId,
        const std::string& eventId) = 0;
};

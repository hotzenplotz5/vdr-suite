#pragma once

#include "SuiteBridgeReadTransportStatus.h"

#include <string>

struct SuiteBridgeMetadataCommandReply
{
    bool transportSucceeded = false;
    int replyCode = 0;
    std::string payload;
    SuiteBridgeReadTransportStatus transportStatus =
        SuiteBridgeReadTransportStatus::Failed;
};

class ISuiteBridgeMetadataTransport
{
public:
    virtual ~ISuiteBridgeMetadataTransport() = default;

    virtual SuiteBridgeMetadataCommandReply requestMetadata(
        const std::string& channelId,
        const std::string& eventId) = 0;
};

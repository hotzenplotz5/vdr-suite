#pragma once

#include "SuiteBridgeReadTransportStatus.h"

#include <string>

struct SuiteBridgeRecordingMetadataCommandReply
{
    bool transportSucceeded = false;
    SuiteBridgeReadTransportStatus transportStatus =
        SuiteBridgeReadTransportStatus::Failed;
    int replyCode = 0;
    std::string payload;
};

class ISuiteBridgeRecordingMetadataTransport
{
public:
    virtual ~ISuiteBridgeRecordingMetadataTransport() = default;

    virtual SuiteBridgeRecordingMetadataCommandReply
    requestRecordingMetadata(
        const std::string& recordingKey) = 0;
};

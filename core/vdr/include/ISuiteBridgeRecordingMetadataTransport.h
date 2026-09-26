#pragma once

#include "SuiteBridgeReadTransportStatus.h"

#include <string>

struct SuiteBridgeRecordingMetadataCommandReply
{
    bool transportSucceeded = false;
    int replyCode = 0;
    std::string payload;
    SuiteBridgeReadTransportStatus transportStatus =
        SuiteBridgeReadTransportStatus::Failed;
};

class ISuiteBridgeRecordingMetadataTransport
{
public:
    virtual ~ISuiteBridgeRecordingMetadataTransport() = default;

    virtual SuiteBridgeRecordingMetadataCommandReply
    requestRecordingMetadata(
        const std::string& recordingKey) = 0;
};

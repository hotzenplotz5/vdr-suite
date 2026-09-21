#pragma once

#include <string>

struct SuiteBridgeRecordingMetadataCommandReply
{
    bool transportSucceeded = false;
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

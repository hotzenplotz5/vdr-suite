#pragma once

#include <string>

struct SuiteBridgeRecordingCutStateCommandReply
{
    bool transportSucceeded = false;
    int replyCode = 0;
    std::string payload;
};

class ISuiteBridgeRecordingCutStateTransport
{
public:
    virtual ~ISuiteBridgeRecordingCutStateTransport() = default;

    virtual SuiteBridgeRecordingCutStateCommandReply requestRecordingCutState(
        const std::string& recordingKey) = 0;
};

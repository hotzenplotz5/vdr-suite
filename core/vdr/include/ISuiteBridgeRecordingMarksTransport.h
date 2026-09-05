#pragma once

#include <string>

struct SuiteBridgeRecordingMarksCommandReply
{
    bool transportSucceeded = false;
    int replyCode = 0;
    std::string payload;
};

class ISuiteBridgeRecordingMarksTransport
{
public:
    virtual ~ISuiteBridgeRecordingMarksTransport() = default;

    virtual SuiteBridgeRecordingMarksCommandReply requestRecordingMarks(
        const std::string& recordingKey) = 0;
};

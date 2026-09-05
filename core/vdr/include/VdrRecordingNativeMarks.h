#pragma once

#include <cstddef>
#include <string>
#include <vector>

enum class VdrRecordingNativeMarksAvailability
{
    Available,
    RecordingNotFound,
    NativeUnreadable,
    TransportError,
    InvalidPayload
};

struct VdrRecordingNativeMark
{
    int positionFrame = 0;
    std::string timecode;
    double positionSeconds = 0.0;
    std::string comment;
};

struct VdrRecordingNativeMarks
{
    static constexpr int SupportedSchema = 1;
    static constexpr int SupportedIdentitySchema = 1;
    static constexpr std::size_t MaximumMarks = 2048;
    static constexpr std::size_t MaximumCommentBytes = 1024;
    static constexpr std::size_t MaximumPayloadBytes = 65535;

    VdrRecordingNativeMarksAvailability availability =
        VdrRecordingNativeMarksAvailability::InvalidPayload;
    int schema = 0;
    bool found = false;
    std::string reason;
    int recordingIdentitySchema = 0;
    std::string recordingKey;
    std::string state;
    double framesPerSecond = 0.0;
    bool isPesRecording = false;
    int inUseFlags = 0;
    bool marksFilePresent = false;
    int sequenceCount = 0;
    std::string marksRevision;
    std::vector<VdrRecordingNativeMark> marks;
    std::string diagnostic;
};

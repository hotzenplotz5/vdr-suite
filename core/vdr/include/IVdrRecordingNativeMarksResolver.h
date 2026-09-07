#pragma once

#include "VdrRecordingNativeMarks.h"

#include <string>

class IVdrRecordingNativeMarksResolver
{
public:
    virtual ~IVdrRecordingNativeMarksResolver() = default;

    virtual VdrRecordingNativeMarks resolve(
        const std::string& recordingKey) = 0;
};

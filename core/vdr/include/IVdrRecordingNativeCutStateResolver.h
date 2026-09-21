#pragma once

#include "VdrRecordingNativeCutState.h"

#include <string>

class IVdrRecordingNativeCutStateResolver
{
public:
    virtual ~IVdrRecordingNativeCutStateResolver() = default;

    virtual VdrRecordingNativeCutState resolve(
        const std::string& recordingKey) = 0;
};

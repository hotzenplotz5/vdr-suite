#pragma once

#include "VdrRecordingNativeMetadata.h"

#include <string>

class IVdrRecordingNativeMetadataResolver
{
public:
    virtual ~IVdrRecordingNativeMetadataResolver() = default;

    virtual VdrRecordingNativeMetadata resolve(
        const std::string& recordingKey) = 0;
};

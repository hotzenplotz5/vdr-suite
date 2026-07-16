#pragma once

#include "VdrRecordingMetadata.h"

#include <string>

class VdrRecordingMetadataCacheCodec
{
public:
    static std::string encode(
        const VdrRecordingMetadata& metadata);

    static VdrRecordingMetadata decode(
        const std::string& payload);
};

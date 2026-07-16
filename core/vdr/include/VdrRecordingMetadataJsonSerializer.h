#pragma once

#include "VdrRecording.h"

#include <string>

class VdrRecordingMetadataJsonSerializer
{
public:
    static std::string serialize(
        const VdrRecording& recording);
};

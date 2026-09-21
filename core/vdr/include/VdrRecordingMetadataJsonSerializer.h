#pragma once

#include "VdrRecording.h"

#include <string>

class VdrRecordingMetadataJsonSerializer
{
public:
    static std::string presentationTitle(
        const VdrRecording& recording);

    static std::string presentationSubtitle(
        const VdrRecording& recording);

    static std::string preferredArtworkUrl(
        const VdrRecording& recording);

    static std::string serialize(
        const VdrRecording& recording);
};

#include "VdrRecordingMetadataJsonSerializer.inl"

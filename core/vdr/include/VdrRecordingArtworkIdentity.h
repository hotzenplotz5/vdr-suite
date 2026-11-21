#pragma once

#include "VdrRecording.h"

#include <string>

class VdrRecordingArtworkIdentity
{
public:
    static std::string assetId(
        const VdrRecording& recording,
        const VdrRecordingArtworkRef& artwork);

    static const VdrRecordingArtworkRef* preferredArtwork(
        const VdrRecording& recording);

    static std::string publicUrl(
        const VdrRecording& recording,
        const VdrRecordingArtworkRef& artwork);

    static bool isValidAssetId(
        const std::string& assetId);
};

#pragma once

#include "VdrRecordingQueryResult.h"

#include <functional>
#include <string>

class VdrRecordingQueryResultJsonSerializer
{
public:
    using MetadataOverlaySerializer =
        std::function<std::string(const VdrRecording& recording)>;

    std::string serialize(
        const VdrRecordingQueryResult& result) const;

    std::string serialize(
        const VdrRecordingQueryResult& result,
        const MetadataOverlaySerializer& metadataOverlaySerializer) const;
};

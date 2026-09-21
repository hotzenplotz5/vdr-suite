#pragma once

#include "VdrRecordingMetadata.h"

#include <string>

// Internal versioned payload format for the dedicated metadata sidecar cache.
// Defining and testing the codec does not migrate the current Recording table;
// that persistence step remains a separately reviewed Phase 60.15 slice.
class VdrRecordingMetadataCacheCodec
{
public:
    static std::string encode(
        const VdrRecordingMetadata& metadata);

    static VdrRecordingMetadata decode(
        const std::string& payload);
};

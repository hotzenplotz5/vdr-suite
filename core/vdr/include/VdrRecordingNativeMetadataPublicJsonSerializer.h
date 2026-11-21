#pragma once

#include "VdrRecordingNativeMetadataRepository.h"

#include <string>

class VdrRecordingNativeMetadataPublicJsonSerializer
{
public:
    std::string serialize(
        const VdrRecordingNativeMetadataRecord& record) const;

    static std::string imageUrl(
        const VdrRecordingNativeMetadataRecord& record,
        const std::string& kind,
        int index);
};

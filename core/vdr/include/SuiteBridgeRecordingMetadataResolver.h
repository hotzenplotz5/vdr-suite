#pragma once

#include "ISuiteBridgeRecordingMetadataTransport.h"
#include "VdrRecordingNativeMetadata.h"

#include <string>

class SuiteBridgeRecordingMetadataResolver final
{
public:
    explicit SuiteBridgeRecordingMetadataResolver(
        ISuiteBridgeRecordingMetadataTransport& transport);

    VdrRecordingNativeMetadata resolve(
        const std::string& recordingKey);

    static VdrRecordingNativeMetadata parseReply(
        const std::string& expectedRecordingKey,
        const SuiteBridgeRecordingMetadataCommandReply& reply);

private:
    ISuiteBridgeRecordingMetadataTransport& transport_;
};

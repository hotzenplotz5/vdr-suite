#pragma once

#include "ISuiteBridgeRecordingMetadataTransport.h"
#include "IVdrRecordingNativeMetadataResolver.h"

#include <string>

class SuiteBridgeRecordingMetadataResolver final :
    public IVdrRecordingNativeMetadataResolver
{
public:
    explicit SuiteBridgeRecordingMetadataResolver(
        ISuiteBridgeRecordingMetadataTransport& transport);

    VdrRecordingNativeMetadata resolve(
        const std::string& recordingKey) override;

    static VdrRecordingNativeMetadata parseReply(
        const std::string& expectedRecordingKey,
        const SuiteBridgeRecordingMetadataCommandReply& reply);

private:
    ISuiteBridgeRecordingMetadataTransport& transport_;
};

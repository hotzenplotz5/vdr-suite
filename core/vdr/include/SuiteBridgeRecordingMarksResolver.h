#pragma once

#include "ISuiteBridgeRecordingMarksTransport.h"
#include "IVdrRecordingNativeMarksResolver.h"

#include <string>

class SuiteBridgeRecordingMarksResolver final :
    public IVdrRecordingNativeMarksResolver
{
public:
    explicit SuiteBridgeRecordingMarksResolver(
        ISuiteBridgeRecordingMarksTransport& transport);

    VdrRecordingNativeMarks resolve(
        const std::string& recordingKey) override;

    static VdrRecordingNativeMarks parseReply(
        const std::string& expectedRecordingKey,
        const SuiteBridgeRecordingMarksCommandReply& reply);

private:
    ISuiteBridgeRecordingMarksTransport& transport_;
};

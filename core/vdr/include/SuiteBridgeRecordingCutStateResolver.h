#pragma once

#include "ISuiteBridgeRecordingCutStateTransport.h"
#include "IVdrRecordingNativeCutStateResolver.h"

#include <string>

class SuiteBridgeRecordingCutStateResolver final :
    public IVdrRecordingNativeCutStateResolver
{
public:
    explicit SuiteBridgeRecordingCutStateResolver(
        ISuiteBridgeRecordingCutStateTransport& transport);

    VdrRecordingNativeCutState resolve(
        const std::string& recordingKey) override;

    static VdrRecordingNativeCutState parseReply(
        const std::string& expectedRecordingKey,
        const SuiteBridgeRecordingCutStateCommandReply& reply);

private:
    ISuiteBridgeRecordingCutStateTransport& transport_;
};

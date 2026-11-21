#pragma once

#include "NativeTimerSpecification.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

struct NativeTimerCreateOperationPayload
{
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t assignmentEpoch = 0;

    std::string nativeTimerBindingId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;

    NativeTimerSpecification expectedSpecification;
};

bool nativeTimerCreateOperationPayloadValid(
    const NativeTimerCreateOperationPayload& payload);

std::string serializeNativeTimerCreateOperationPayload(
    const NativeTimerCreateOperationPayload& payload);

bool parseNativeTimerCreateOperationPayload(
    const std::string& serialized,
    NativeTimerCreateOperationPayload& payload);

std::string nativeTimerCreateOperationPayloadFingerprint(
    const NativeTimerCreateOperationPayload& payload);

}

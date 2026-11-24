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
    // Provider-local CREATE metadata, not part of the Binding or preview.
    std::string aux;
};

inline bool nativeTimerCreateAuxValid(const std::string& aux)
{
    return aux.size() <= 1024U &&
        aux.find_first_of("\0\r\n", 0U, 3U) == std::string::npos &&
        aux.find("<vdr-suite-managed-timer") == std::string::npos;
}

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

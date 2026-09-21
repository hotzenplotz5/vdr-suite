#pragma once

#include "NativeTimerSpecification.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

enum class NativeTimerModifyKind
{
    update,
    toggle,
};

const char* nativeTimerModifyKindName(NativeTimerModifyKind kind);

struct NativeTimerModifyOperationPayload
{
    NativeTimerModifyKind kind = NativeTimerModifyKind::update;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t assignmentEpoch = 0;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::string expectedCurrentFingerprint;
    NativeTimerSpecification expectedSpecification;
};

bool nativeTimerModifyOperationPayloadValid(
    const NativeTimerModifyOperationPayload& payload);
std::string serializeNativeTimerModifyOperationPayload(
    const NativeTimerModifyOperationPayload& payload);
bool parseNativeTimerModifyOperationPayload(
    const std::string& serialized,
    NativeTimerModifyOperationPayload& payload);
std::string nativeTimerModifyOperationPayloadFingerprint(
    const NativeTimerModifyOperationPayload& payload);

}

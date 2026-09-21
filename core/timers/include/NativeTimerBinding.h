#pragma once

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

enum class NativeTimerBindingOwnership
{
    managed,
    adopted,
    external,
    orphanedManaged,
    ambiguous
};

enum class NativeTimerBindingDriftState
{
    none,
    expectedTransition,
    externalFieldChange,
    externalDisable,
    externalDelete,
    nativeIdentityChanged,
    ambiguous
};

// Normalized copied native Timer facts. This value intentionally does not
// depend on the VDR adapter/read-model type and does not expose plugin-specific
// opaque aux content as a cross-backend domain field.
struct NativeTimerObservedState
{
    std::string channelId;
    std::string eventId;
    std::string title;
    std::string directory;
    std::string day;
    std::string weekdays;
    std::string startTime;
    std::string endTime;
    std::int32_t flags = 0;
    std::int32_t priority = 50;
    std::int32_t lifetime = 99;
    bool enabled = true;
    bool vps = false;
    bool recording = false;
    bool pending = false;
};

struct NativeTimerBinding
{
    std::string nativeTimerBindingId;
    std::string bindingRevision;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::string timerAssignmentId;
    NativeTimerBindingOwnership ownership =
        NativeTimerBindingOwnership::external;
    std::string observedFingerprint;
    NativeTimerObservedState observedState;
    std::int64_t lastObservedAt = 0;
    std::string lastVerifiedOperationId;
    std::int64_t missingSince = 0;
    NativeTimerBindingDriftState driftState =
        NativeTimerBindingDriftState::none;
};

const char* nativeTimerBindingOwnershipName(
    NativeTimerBindingOwnership ownership);
const char* nativeTimerBindingDriftStateName(
    NativeTimerBindingDriftState driftState);

bool nativeTimerObservedStateValid(
    const NativeTimerObservedState& state);
std::string nativeTimerObservedStateFingerprint(
    const NativeTimerObservedState& state);

bool nativeTimerBindingValid(const NativeTimerBinding& binding);
bool nativeTimerBindingRevisionMatches(
    const std::string& expectedRevision,
    const std::string& currentRevision);

}

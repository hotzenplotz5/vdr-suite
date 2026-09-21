#include "NativeTimerBinding.h"

#include <cstddef>
#include <string>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxTextLength = 1024;
constexpr std::size_t kMaxFingerprintLength = 4096;

bool bounded(const std::string& value, std::size_t maximum)
{
    return value.size() <= maximum;
}

bool nonEmptyBounded(const std::string& value, std::size_t maximum)
{
    return !value.empty() && bounded(value, maximum);
}

bool validWeekdays(const std::string& value)
{
    if (value.size() != 7) return false;
    for (char ch : value)
    {
        const bool letter = (ch >= 'A' && ch <= 'Z')
            || (ch >= 'a' && ch <= 'z');
        if (ch != '-' && !letter) return false;
    }
    return true;
}

bool validNativeHhmm(const std::string& value)
{
    if (value.empty() || value.size() > 4) return false;

    int numeric = 0;
    for (char ch : value)
    {
        if (ch < '0' || ch > '9') return false;
        numeric = numeric * 10 + (ch - '0');
    }

    const int hour = numeric / 100;
    const int minute = numeric % 100;
    return hour <= 23 && minute <= 59;
}

std::string normalizedNativeHhmm(const std::string& value)
{
    if (!validNativeHhmm(value)) return {};

    std::string normalized(4 - value.size(), '0');
    normalized.append(value);
    return normalized;
}

void appendField(std::string& output, const std::string& value)
{
    output.append(std::to_string(value.size()));
    output.push_back(':');
    output.append(value);
    output.push_back('|');
}

void appendInteger(std::string& output, std::int64_t value)
{
    appendField(output, std::to_string(value));
}

void appendBoolean(std::string& output, bool value)
{
    appendField(output, value ? "1" : "0");
}

bool assignmentOwnershipShapeValid(const NativeTimerBinding& binding)
{
    switch (binding.ownership)
    {
        case NativeTimerBindingOwnership::managed:
        case NativeTimerBindingOwnership::adopted:
            return nonEmptyBounded(
                binding.timerAssignmentId,
                kMaxIdentityLength);
        case NativeTimerBindingOwnership::external:
            return binding.timerAssignmentId.empty()
                && binding.lastVerifiedOperationId.empty();
        case NativeTimerBindingOwnership::orphanedManaged:
        case NativeTimerBindingOwnership::ambiguous:
            return bounded(binding.timerAssignmentId, kMaxIdentityLength);
    }
    return false;
}

bool missingDriftShapeValid(const NativeTimerBinding& binding)
{
    if (binding.missingSince == 0)
    {
        return binding.driftState
            != NativeTimerBindingDriftState::externalDelete;
    }

    if (binding.missingSince < 0
        || binding.missingSince > binding.lastObservedAt)
    {
        return false;
    }

    return binding.driftState
            == NativeTimerBindingDriftState::expectedTransition
        || binding.driftState
            == NativeTimerBindingDriftState::externalDelete
        || binding.driftState
            == NativeTimerBindingDriftState::ambiguous;
}
}

const char* nativeTimerBindingOwnershipName(
    NativeTimerBindingOwnership ownership)
{
    switch (ownership)
    {
        case NativeTimerBindingOwnership::managed: return "managed";
        case NativeTimerBindingOwnership::adopted: return "adopted";
        case NativeTimerBindingOwnership::external: return "external";
        case NativeTimerBindingOwnership::orphanedManaged:
            return "orphaned_managed";
        case NativeTimerBindingOwnership::ambiguous: return "ambiguous";
    }
    return "ambiguous";
}

const char* nativeTimerBindingDriftStateName(
    NativeTimerBindingDriftState driftState)
{
    switch (driftState)
    {
        case NativeTimerBindingDriftState::none: return "none";
        case NativeTimerBindingDriftState::expectedTransition:
            return "expected_transition";
        case NativeTimerBindingDriftState::externalFieldChange:
            return "external_field_change";
        case NativeTimerBindingDriftState::externalDisable:
            return "external_disable";
        case NativeTimerBindingDriftState::externalDelete:
            return "external_delete";
        case NativeTimerBindingDriftState::nativeIdentityChanged:
            return "native_identity_changed";
        case NativeTimerBindingDriftState::ambiguous:
            return "ambiguous";
    }
    return "ambiguous";
}

bool nativeTimerObservedStateValid(
    const NativeTimerObservedState& state)
{
    return nonEmptyBounded(state.channelId, kMaxIdentityLength)
        && bounded(state.eventId, kMaxIdentityLength)
        && bounded(state.title, kMaxTextLength)
        && bounded(state.directory, kMaxTextLength)
        && bounded(state.day, kMaxIdentityLength)
        && validWeekdays(state.weekdays)
        && validNativeHhmm(state.startTime)
        && validNativeHhmm(state.endTime)
        && state.flags >= 0
        && state.priority >= 0
        && state.priority <= 99
        && state.lifetime >= 0
        && state.lifetime <= 99;
}

std::string nativeTimerObservedStateFingerprint(
    const NativeTimerObservedState& state)
{
    if (!nativeTimerObservedStateValid(state)) return {};

    std::string fingerprint = "native-timer-observed-state/1|";
    appendField(fingerprint, state.channelId);
    appendField(fingerprint, state.eventId);
    appendField(fingerprint, state.title);
    appendField(fingerprint, state.directory);
    appendField(fingerprint, state.day);
    appendField(fingerprint, state.weekdays);
    appendField(fingerprint, normalizedNativeHhmm(state.startTime));
    appendField(fingerprint, normalizedNativeHhmm(state.endTime));
    appendInteger(fingerprint, state.flags);
    appendInteger(fingerprint, state.priority);
    appendInteger(fingerprint, state.lifetime);
    appendBoolean(fingerprint, state.enabled);
    appendBoolean(fingerprint, state.vps);
    appendBoolean(fingerprint, state.recording);
    appendBoolean(fingerprint, state.pending);
    return fingerprint;
}

bool nativeTimerBindingValid(const NativeTimerBinding& binding)
{
    if (!nonEmptyBounded(
            binding.nativeTimerBindingId,
            kMaxIdentityLength)
        || !nonEmptyBounded(
            binding.bindingRevision,
            kMaxIdentityLength)
        || !nonEmptyBounded(binding.backendId, kMaxIdentityLength)
        || binding.backendGeneration == 0
        || !nonEmptyBounded(
            binding.backendNativeTimerId,
            kMaxIdentityLength)
        || !bounded(binding.timerAssignmentId, kMaxIdentityLength)
        || !nonEmptyBounded(
            binding.observedFingerprint,
            kMaxFingerprintLength)
        || !nativeTimerObservedStateValid(binding.observedState)
        || binding.lastObservedAt <= 0
        || !bounded(
            binding.lastVerifiedOperationId,
            kMaxIdentityLength))
    {
        return false;
    }

    if (binding.observedFingerprint
        != nativeTimerObservedStateFingerprint(binding.observedState))
    {
        return false;
    }

    if (!assignmentOwnershipShapeValid(binding)) return false;
    if (!missingDriftShapeValid(binding)) return false;

    if (binding.driftState == NativeTimerBindingDriftState::externalDisable
        && (binding.missingSince != 0 || binding.observedState.enabled))
    {
        return false;
    }

    return true;
}

bool nativeTimerBindingRevisionMatches(
    const std::string& expectedRevision,
    const std::string& currentRevision)
{
    return !expectedRevision.empty()
        && !currentRevision.empty()
        && expectedRevision == currentRevision;
}

}

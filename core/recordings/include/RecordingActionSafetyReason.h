#pragma once

#include <string>

enum class RecordingActionSafetyReason
{
    Allowed,
    UnsupportedAction,
    BackendUnavailable,
    BackendReadOnly,
    MissingCapability,
    PermissionDenied,
    RecordingInUse,
    ExecutionDisabled
};

inline std::string toString(
    RecordingActionSafetyReason reason)
{
    switch (reason)
    {
    case RecordingActionSafetyReason::Allowed:
        return "allowed";
    case RecordingActionSafetyReason::UnsupportedAction:
        return "unsupported_action";
    case RecordingActionSafetyReason::BackendUnavailable:
        return "backend_unavailable";
    case RecordingActionSafetyReason::BackendReadOnly:
        return "backend_read_only";
    case RecordingActionSafetyReason::MissingCapability:
        return "missing_capability";
    case RecordingActionSafetyReason::PermissionDenied:
        return "permission_denied";
    case RecordingActionSafetyReason::RecordingInUse:
        return "recording_in_use";
    case RecordingActionSafetyReason::ExecutionDisabled:
        return "execution_disabled";
    }

    return "unsupported_action";
}

#include "DaemonRecordingCutReconciliation.h"

#include "VdrRecordingNativeIdentity.h"

bool daemonRecordingCutResultMatches(
    const std::string& expectedSourceRecordingKey,
    const std::string& expectedEditedRecordingKey,
    const VdrRecordingNativeCutState& state) noexcept
{
    return VdrRecordingNativeIdentity::isValidKey(expectedSourceRecordingKey) &&
        VdrRecordingNativeIdentity::isValidKey(expectedEditedRecordingKey) &&
        state.availability == VdrRecordingNativeCutStateAvailability::Available &&
        state.found &&
        state.recordingKey == expectedSourceRecordingKey &&
        state.editedRecordingFound &&
        state.editedDestinationExists &&
        state.editedRecordingKey == expectedEditedRecordingKey;
}

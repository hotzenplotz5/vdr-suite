#pragma once

#include "VdrRecordingNativeCutState.h"

#include <string>

bool daemonRecordingCutResultMatches(
    const std::string& expectedSourceRecordingKey,
    const std::string& expectedEditedRecordingKey,
    const VdrRecordingNativeCutState& state) noexcept;

bool daemonRecordingCutVerifiedResultWasRemoved(
    const std::string& expectedSourceRecordingKey,
    const std::string& expectedEditedRecordingKey,
    const VdrRecordingNativeCutState& state) noexcept;

bool daemonRecordingCutStateBlocksSourceDelete(
    const VdrRecordingNativeCutState& state) noexcept;

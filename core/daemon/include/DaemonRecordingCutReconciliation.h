#pragma once

#include "VdrRecordingNativeCutState.h"

#include <cstdint>
#include <string>

bool daemonRecordingCutResultMatches(
    const std::string& expectedSourceRecordingKey,
    const std::string& expectedEditedRecordingKey,
    const VdrRecordingNativeCutState& state) noexcept;

bool daemonRecordingCutVerifiedResultWasRemoved(
    const std::string& expectedSourceRecordingKey,
    const std::string& expectedEditedRecordingKey,
    const VdrRecordingNativeCutState& state) noexcept;

bool daemonRecordingCutAcceptedStartExpired(
    const std::string& expectedSourceRecordingKey,
    const std::string& expectedEditedRecordingKey,
    std::int64_t claimedAt,
    std::int64_t now,
    const VdrRecordingNativeCutState& state) noexcept;

bool daemonRecordingCutStateBlocksSourceDelete(
    const VdrRecordingNativeCutState& state) noexcept;

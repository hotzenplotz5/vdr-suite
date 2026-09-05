#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerCreateLocalState.h"
#include "BackendAgentNativeTimerDeleteLocalState.h"
#include "BackendAgentNativeTimerModifyLocalState.h"
#include "BackendAgentRecordingMarksModifyLocalState.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

inline constexpr const char* kBackendAgentNativeTimerCreateLocalStateExtensionType =
    "vdr.timer.create.local-state.v1";
inline constexpr const char* kBackendAgentNativeTimerDeleteLocalStateExtensionType =
    "vdr.timer.delete.local-state.v1";
inline constexpr const char* kBackendAgentNativeTimerModifyLocalStateExtensionType =
    "vdr.timer.modify.local-state.v1";
inline constexpr const char* kBackendAgentRecordingMarksModifyLocalStateExtensionType =
    "vdr.recording.marks.modify.local-state.v1";

struct BackendAgentCommandStateExtension
{
    std::uint64_t schemaVersion = 1;
    std::string extensionType;
    std::string commandId;
    std::string requestFingerprint;
    std::string payload;
};

bool backendAgentCommandStateExtensionValid(
    const BackendAgentCommandStateExtension& extension,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode);

std::string backendAgentCommandStateExtensionSerialize(
    const BackendAgentCommandStateExtension& extension,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode);

bool backendAgentCommandStateExtensionParse(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentCommandStateExtension& extension,
    std::string& reasonCode);

bool backendAgentCommandStateExtensionValidateSupported(
    const BackendAgentCommandStateExtension& extension,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode);

std::string backendAgentNativeTimerCreateCommandStateExtension(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerCreateParseCommandStateExtension(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode);

std::string backendAgentNativeTimerDeleteCommandStateExtension(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteParseCommandStateExtension(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode);

std::string backendAgentNativeTimerModifyCommandStateExtension(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerModifyLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerModifyParseCommandStateExtension(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerModifyLocalState& state,
    std::string& reasonCode);

std::string backendAgentRecordingMarksModifyCommandStateExtension(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentRecordingMarksModifyLocalState& state,
    std::string& reasonCode);

bool backendAgentRecordingMarksModifyParseCommandStateExtension(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentRecordingMarksModifyLocalState& state,
    std::string& reasonCode);

}

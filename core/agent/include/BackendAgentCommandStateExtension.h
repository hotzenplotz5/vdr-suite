#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerDeleteLocalState.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

inline constexpr const char* kBackendAgentNativeTimerDeleteLocalStateExtensionType =
    "vdr.timer.delete.local-state.v1";

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

std::string backendAgentNativeTimerDeleteCommandStateExtension(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteParseCommandStateExtension(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode);

}

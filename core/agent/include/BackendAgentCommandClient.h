#pragma once

#include "BackendAgentCommand.h"

#include <cstdint>
#include <string>
#include <vector>

class IBackendAgentControlPlaneTransport;
namespace vdrsuite::agent
{
class IBackendAgentNativeProbeTransport;
class IBackendAgentNativeTimerDeleteTransport;
class IBackendAgentNativeTimerCreateTransport;
class IBackendAgentNativeTimerModifyTransport;
}

struct BackendAgentCommandClientConfig
{
    std::string statePath;
    std::vector<std::string> commandTypes;
    vdrsuite::agent::IBackendAgentNativeProbeTransport* nativeProbeTransport = nullptr;
    vdrsuite::agent::IBackendAgentNativeTimerDeleteTransport* nativeTimerDeleteTransport = nullptr;
    vdrsuite::agent::IBackendAgentNativeTimerCreateTransport* nativeTimerCreateTransport = nullptr;
    vdrsuite::agent::IBackendAgentNativeTimerModifyTransport* nativeTimerModifyTransport = nullptr;
};

struct BackendAgentCommandClientContext
{
    std::string agentId;
    std::string credentialSecret;
    std::string backendId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
};

bool reconcileBackendAgentCommandState(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    IBackendAgentControlPlaneTransport& transport,
    std::string& reasonCode);

bool pollBackendAgentCommand(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    IBackendAgentControlPlaneTransport& transport,
    std::string& reasonCode);

void setBackendAgentNativeProbeTransport(
    vdrsuite::agent::IBackendAgentNativeProbeTransport* transport);

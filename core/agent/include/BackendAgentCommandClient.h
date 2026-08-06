#pragma once

#include "BackendAgentCommand.h"

#include <cstdint>
#include <string>
#include <vector>

class IBackendAgentControlPlaneTransport;

struct BackendAgentCommandClientConfig
{
    std::string statePath;
    std::vector<std::string> commandTypes;
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

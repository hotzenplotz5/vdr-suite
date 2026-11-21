#pragma once

#include "BackendAgentCommandStateStore.h"
#include "BackendAgentNativeProbe.h"

#include <string>

namespace vdrsuite::agent
{

void backendAgentNativeProbeCommandSetDefaultTransport(
    IBackendAgentNativeProbeTransport* transport);

bool backendAgentNativeProbeCommandAvailability(
    IBackendAgentNativeProbeTransport* configuredTransport,
    BackendAgentLocalProviderFacts& facts,
    std::string& reasonCode);

bool backendAgentNativeProbeCommandReconcile(
    const std::string& statePath,
    IBackendAgentNativeProbeTransport* configuredTransport,
    commandstate::LocalState& state,
    std::string& reasonCode);

}

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
class IBackendAgentRecordingMarksModifyTransport;
class IBackendAgentRecordingCutTransport;

// Production-only default binding for the native recording-cut transport.
// Tests may continue to inject an explicit per-config transport below. The
// shipped Agent sets this binding only after constructing the loopback
// SuiteBridge transport and capability advertisement remains discovery-gated.
inline IBackendAgentRecordingCutTransport* RecordingCutDefaultTransport = nullptr;
}

struct BackendAgentCommandClientConfig
{
    std::string statePath;
    std::vector<std::string> commandTypes;
    vdrsuite::agent::IBackendAgentNativeProbeTransport* nativeProbeTransport = nullptr;
    vdrsuite::agent::IBackendAgentNativeTimerDeleteTransport* nativeTimerDeleteTransport = nullptr;
    vdrsuite::agent::IBackendAgentNativeTimerCreateTransport* nativeTimerCreateTransport = nullptr;
    vdrsuite::agent::IBackendAgentNativeTimerModifyTransport* nativeTimerModifyTransport = nullptr;
    vdrsuite::agent::IBackendAgentRecordingCutTransport* recordingCutTransport =
        vdrsuite::agent::RecordingCutDefaultTransport;
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

void setBackendAgentRecordingMarksModifyTransport(
    vdrsuite::agent::IBackendAgentRecordingMarksModifyTransport* transport);

inline void setBackendAgentRecordingCutTransport(
    vdrsuite::agent::IBackendAgentRecordingCutTransport* transport)
{
    vdrsuite::agent::RecordingCutDefaultTransport = transport;
}

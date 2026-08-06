#pragma once

#include "ISuiteBridgeLocalTransport.h"

#include <cstdint>
#include <string>

struct BackendAgentCommandAssignment;

namespace vdrsuite::agent
{

struct SuiteBridgeNativeProbeCapability
{
    std::string nativeOperation;
    std::uint64_t nativeOperationSchema = 0;
    std::string sideEffectClass;
    std::string mutations;
    std::string localProviderKind;
    std::string pluginInstanceEpoch;
};

struct SuiteBridgeNativeProbeRequest
{
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string pluginInstanceEpoch;
    std::string probeNonce;
};

struct SuiteBridgeNativeProbeReadbackRequest
{
    std::string commandId;
    std::string requestFingerprint;
    std::string pluginInstanceEpoch;
    std::uint64_t nativeExecutionSequence = 0;
};

struct SuiteBridgeNativeProbeEvidence
{
    std::string commandId;
    std::string requestFingerprint;
    std::string nativeOperation;
    std::uint64_t nativeOperationSchema = 0;
    std::string pluginInstanceEpoch;
    std::uint64_t nativeExecutionSequence = 0;
    std::string receiptCategory;
    std::int64_t acceptedAt = 0;
    std::string sideEffectClass;
    std::string resultCategory;
    bool vdrActive = false;
    std::string mutationsState;
    bool sideEffectObserved = true;
    std::string boundedDiagnostics;
    std::int64_t completedAt = 0;
    std::string readbackCategory;
    std::string duplicateDisposition;
};

class IBackendAgentNativeProbeTransport
{
public:
    virtual ~IBackendAgentNativeProbeTransport() = default;
    virtual SuiteBridgeCommandReply discoverNativeProbe() = 0;
    virtual SuiteBridgeCommandReply executeNativeProbe(
        const SuiteBridgeNativeProbeRequest& request) = 0;
    virtual SuiteBridgeCommandReply readNativeProbe(
        const SuiteBridgeNativeProbeReadbackRequest& request) = 0;
};

bool backendAgentNativeProbeParsePayload(
    const std::string& payload,
    std::string& probeNonce);

bool backendAgentNativeProbeParseCapability(
    const std::string& payload,
    SuiteBridgeNativeProbeCapability& capability,
    std::string& reasonCode);

bool backendAgentNativeProbeParseEvidence(
    const std::string& payload,
    bool requireReadback,
    SuiteBridgeNativeProbeEvidence& evidence,
    std::string& reasonCode);

bool backendAgentNativeProbeCapabilityCompatible(
    const SuiteBridgeNativeProbeCapability& capability);

bool backendAgentNativeProbeEvidenceMatches(
    const SuiteBridgeNativeProbeEvidence& evidence,
    const SuiteBridgeNativeProbeRequest& request,
    bool requireReadback);

std::string backendAgentNativeProbeCapabilityEvidence(
    const SuiteBridgeNativeProbeCapability& capability);

std::string backendAgentNativeProbeReceiptEvidence(
    const SuiteBridgeNativeProbeEvidence& evidence);

std::string backendAgentNativeProbeResultEvidence(
    const SuiteBridgeNativeProbeEvidence& evidence);

std::string backendAgentNativeProbeReadbackEvidence(
    const SuiteBridgeNativeProbeEvidence& evidence);

}

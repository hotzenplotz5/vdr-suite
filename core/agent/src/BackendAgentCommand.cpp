#include "BackendAgentCommand.h"
#include "BackendAgentNativeProbe.h"
#include "BackendAgentNativeTimerCreatePayload.h"
#include "BackendAgentNativeTimerDeletePayload.h"
#include "BackendAgentNativeTimerModifyPayload.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>

namespace
{
std::string stableIdentity(const std::string& value)
{
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char character : value)
    {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    std::ostringstream output;
    output << "fp1_" << std::hex << std::setw(16) << std::setfill('0') << hash;
    return output.str();
}

void appendField(std::ostringstream& output, const std::string& value)
{
    output << value.size() << ':' << value << '|';
}

void appendField(std::ostringstream& output, std::uint64_t value)
{
    output << value << '|';
}

void appendField(std::ostringstream& output, std::int64_t value)
{
    output << value << '|';
}

bool allowed(const std::string& value, const std::vector<std::string>& values)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool validCommandPayload(const BackendAgentCommandAssignment& value)
{
    if (value.commandType == "probe.noop")
    {
        return value.payloadVersion == 1 && value.payload == "{}" &&
            value.verificationPolicy == "none";
    }
    if (value.commandType == "vdr.native.probe" &&
        value.verificationPolicy == "readback_required")
    {
        if (value.payloadVersion == 1)
        {
            std::string probeNonce;
            return vdrsuite::agent::backendAgentNativeProbeParsePayload(
                value.payload, probeNonce);
        }
        if (value.payloadVersion == 2)
        {
            vdrsuite::agent::BackendAgentNativeProbePayload payload;
            std::string reason;
            return vdrsuite::agent::backendAgentNativeProbeParseSelectedPayload(
                value.payload, payload, reason);
        }
    }
    if (value.commandType ==
            vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType &&
        value.payloadVersion ==
            vdrsuite::agent::kBackendAgentNativeTimerCreatePayloadVersion &&
        value.verificationPolicy == "readback_required")
    {
        vdrsuite::agent::BackendAgentNativeTimerCreatePayload payload;
        std::string reason;
        return vdrsuite::agent::backendAgentNativeTimerCreateParsePayload(
                   value.payload, payload, reason) &&
            payload.localProviderSelection.backendId == value.backendId &&
            payload.controlPlaneClaimedAt <= value.assignedAt;
    }
    if (value.commandType ==
            vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType &&
        value.payloadVersion ==
            vdrsuite::agent::kBackendAgentNativeTimerDeletePayloadVersion &&
        value.verificationPolicy == "readback_required")
    {
        vdrsuite::agent::BackendAgentNativeTimerDeletePayload payload;
        std::string reason;
        return vdrsuite::agent::backendAgentNativeTimerDeleteParsePayload(
                   value.payload, payload, reason) &&
            payload.localProviderSelection.backendId == value.backendId &&
            payload.controlPlaneClaimedAt <= value.assignedAt;
    }
    return false;
}
}

bool backendAgentCommandSafeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' || character == '_' ||
            character == '.' || character == ':';
    });
}

bool backendAgentCommandSafeText(const std::string& value, std::size_t maximumBytes)
{
    return value.size() <= maximumBytes &&
        std::none_of(value.begin(), value.end(), [](unsigned char character) {
            return character < 0x20U || character == 0x7fU;
        });
}

std::string backendAgentCommandFingerprint(const BackendAgentCommandAssignment& assignment)
{
    std::ostringstream canonical;
    appendField(canonical, assignment.protocolVersion);
    appendField(canonical, assignment.requestId);
    appendField(canonical, assignment.correlationId);
    appendField(canonical, assignment.operationId);
    appendField(canonical, assignment.jobId);
    appendField(canonical, assignment.attemptId);
    appendField(canonical, assignment.claimEpoch);
    appendField(canonical, assignment.commandId);
    appendField(canonical, assignment.backendId);
    appendField(canonical, assignment.agentId);
    appendField(canonical, assignment.agentInstanceId);
    appendField(canonical, assignment.backendGeneration);
    appendField(canonical, assignment.commandType);
    appendField(canonical, assignment.payloadVersion);
    appendField(canonical, assignment.payload);
    appendField(canonical, assignment.verificationPolicy);
    appendField(canonical, assignment.assignedAt);
    appendField(canonical, assignment.deadline);
    return stableIdentity(canonical.str());
}

std::string backendAgentCommandReceiptIdentity(const BackendAgentCommandReceipt& receipt)
{
    std::ostringstream canonical;
    appendField(canonical, receipt.protocolVersion);
    appendField(canonical, receipt.commandId);
    appendField(canonical, receipt.requestFingerprint);
    appendField(canonical, receipt.jobId);
    appendField(canonical, receipt.attemptId);
    appendField(canonical, receipt.claimEpoch);
    appendField(canonical, receipt.backendId);
    appendField(canonical, receipt.agentId);
    appendField(canonical, receipt.agentInstanceId);
    appendField(canonical, receipt.backendGeneration);
    appendField(canonical, receipt.receiptCategory);
    appendField(canonical, receipt.receivedAt);
    appendField(canonical, receipt.reasonCode);
    return stableIdentity(canonical.str());
}

std::string backendAgentCommandResultIdentity(const BackendAgentCommandResult& result)
{
    std::ostringstream canonical;
    appendField(canonical, result.protocolVersion);
    appendField(canonical, result.commandId);
    appendField(canonical, result.requestFingerprint);
    appendField(canonical, result.jobId);
    appendField(canonical, result.attemptId);
    appendField(canonical, result.claimEpoch);
    appendField(canonical, result.backendId);
    appendField(canonical, result.agentId);
    appendField(canonical, result.agentInstanceId);
    appendField(canonical, result.backendGeneration);
    appendField(canonical, result.dispatchState);
    appendField(canonical, result.verificationState);
    appendField(canonical, result.resultCategory);
    appendField(canonical, result.errorCategory);
    appendField(canonical, result.retryClassification);
    appendField(canonical, result.boundedDiagnostics);
    appendField(canonical, result.completedAt);
    return stableIdentity(canonical.str());
}

bool backendAgentCommandValidAssignment(const BackendAgentCommandAssignment& value)
{
    return value.protocolVersion == "vdr-suite-agent/1" &&
        backendAgentCommandSafeIdentifier(value.requestId) &&
        backendAgentCommandSafeIdentifier(value.correlationId) &&
        backendAgentCommandSafeIdentifier(value.operationId) &&
        backendAgentCommandSafeIdentifier(value.jobId) &&
        backendAgentCommandSafeIdentifier(value.attemptId) &&
        value.claimEpoch > 0 &&
        backendAgentCommandSafeIdentifier(value.commandId) &&
        backendAgentCommandSafeIdentifier(value.backendId) &&
        backendAgentCommandSafeIdentifier(value.agentId) &&
        backendAgentCommandSafeIdentifier(value.agentInstanceId) &&
        value.backendGeneration > 0 && validCommandPayload(value) &&
        backendAgentCommandSafeIdentifier(value.requestFingerprint) &&
        value.assignedAt > 0 && value.deadline > value.assignedAt &&
        value.requestFingerprint == backendAgentCommandFingerprint(value);
}

bool backendAgentCommandValidReceipt(const BackendAgentCommandReceipt& value)
{
    return value.protocolVersion == "vdr-suite-agent/1" &&
        backendAgentCommandSafeIdentifier(value.commandId) &&
        backendAgentCommandSafeIdentifier(value.requestFingerprint) &&
        backendAgentCommandSafeIdentifier(value.jobId) &&
        backendAgentCommandSafeIdentifier(value.attemptId) && value.claimEpoch > 0 &&
        backendAgentCommandSafeIdentifier(value.backendId) &&
        backendAgentCommandSafeIdentifier(value.agentId) &&
        backendAgentCommandSafeIdentifier(value.agentInstanceId) &&
        value.backendGeneration > 0 && value.receivedAt > 0 &&
        allowed(value.receiptCategory, {"accepted", "duplicate", "rejected", "expired", "stale", "unsupported", "conflict"}) &&
        backendAgentCommandSafeText(value.reasonCode, 256);
}

bool backendAgentCommandValidResult(const BackendAgentCommandResult& value)
{
    return value.protocolVersion == "vdr-suite-agent/1" &&
        backendAgentCommandSafeIdentifier(value.commandId) &&
        backendAgentCommandSafeIdentifier(value.requestFingerprint) &&
        backendAgentCommandSafeIdentifier(value.jobId) &&
        backendAgentCommandSafeIdentifier(value.attemptId) && value.claimEpoch > 0 &&
        backendAgentCommandSafeIdentifier(value.backendId) &&
        backendAgentCommandSafeIdentifier(value.agentId) &&
        backendAgentCommandSafeIdentifier(value.agentInstanceId) &&
        value.backendGeneration > 0 &&
        allowed(value.dispatchState, {"not_started", "starting", "accepted_by_executor", "effect_reported"}) &&
        allowed(value.verificationState, {"not_required", "verified", "outcome_unknown"}) &&
        allowed(value.resultCategory, {"succeeded", "rejected", "outcome_unknown"}) &&
        allowed(value.errorCategory, {"none", "fenced", "expired", "unsupported", "executor_unknown"}) &&
        allowed(value.retryClassification, {"none", "reconcile_only"}) &&
        backendAgentCommandSafeText(value.boundedDiagnostics, 1024) &&
        value.completedAt > 0;
}

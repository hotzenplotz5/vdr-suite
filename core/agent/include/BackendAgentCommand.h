#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct BackendAgentCommandAssignment
{
    bool present = false;
    std::string protocolVersion = "vdr-suite-agent/1";
    std::string requestId;
    std::string correlationId;
    std::string operationId;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string commandId;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string commandType;
    std::uint64_t payloadVersion = 0;
    std::string payload;
    std::string requestFingerprint;
    std::string verificationPolicy;
    std::int64_t assignedAt = 0;
    std::int64_t deadline = 0;
};

struct BackendAgentCommandPollRequest
{
    std::string protocolVersion = "vdr-suite-agent/1";
    std::string backendId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::vector<std::string> supportedCommandTypes;
};

struct BackendAgentCommandPollResult
{
    bool accepted = false;
    std::string reasonCode;
    BackendAgentCommandAssignment assignment;
};

struct BackendAgentCommandReceipt
{
    std::string protocolVersion = "vdr-suite-agent/1";
    std::string commandId;
    std::string requestFingerprint;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string receiptCategory;
    std::int64_t receivedAt = 0;
    std::string reasonCode;
};

struct BackendAgentCommandReceiptResult
{
    bool accepted = false;
    bool replayed = false;
    bool dropResponse = false;
    std::string reasonCode;
};

struct BackendAgentCommandResult
{
    std::string protocolVersion = "vdr-suite-agent/1";
    std::string commandId;
    std::string requestFingerprint;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string dispatchState;
    std::string verificationState;
    std::string resultCategory;
    std::string errorCategory;
    std::string retryClassification;
    std::string boundedDiagnostics;
    std::int64_t completedAt = 0;
};

struct BackendAgentCommandResultAck
{
    bool accepted = false;
    bool replayed = false;
    bool dropResponse = false;
    std::string reasonCode;
};

struct BackendAgentCommandSummary
{
    bool present = false;
    std::string commandId;
    std::string commandType;
    std::string state;
    std::string receiptCategory;
    std::string resultCategory;
    std::string dispatchState;
    std::string verificationState;
    std::uint64_t backendGeneration = 0;
    std::uint64_t claimEpoch = 0;
    std::uint64_t deliveryCount = 0;
    std::uint64_t receiptReplayCount = 0;
    std::uint64_t resultReplayCount = 0;
    std::int64_t deadline = 0;
};

bool backendAgentCommandSafeIdentifier(const std::string& value);
bool backendAgentCommandSafeText(const std::string& value, std::size_t maximumBytes);
bool backendAgentCommandValidAssignment(const BackendAgentCommandAssignment& assignment);
bool backendAgentCommandValidReceipt(const BackendAgentCommandReceipt& receipt);
bool backendAgentCommandValidResult(const BackendAgentCommandResult& result);
std::string backendAgentCommandFingerprint(const BackendAgentCommandAssignment& assignment);
std::string backendAgentCommandReceiptIdentity(const BackendAgentCommandReceipt& receipt);
std::string backendAgentCommandResultIdentity(const BackendAgentCommandResult& result);

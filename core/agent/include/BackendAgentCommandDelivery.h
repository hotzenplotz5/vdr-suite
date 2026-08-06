#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class Database;

enum class BackendAgentCommandDispatchState
{
    NotStarted,
    Starting,
    AcceptedByExecutor,
    EffectReported
};

std::string backendAgentCommandDispatchStateName(
    BackendAgentCommandDispatchState state);

struct BackendAgentCommandEnvelope
{
    std::string protocolVersion;
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
    std::int64_t assignedAt = 0;
    std::int64_t deadline = 0;
};

struct BackendAgentCommandReceipt
{
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

struct BackendAgentCommandResult
{
    std::string commandId;
    std::string requestFingerprint;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    BackendAgentCommandDispatchState dispatchState =
        BackendAgentCommandDispatchState::NotStarted;
    std::string verificationState;
    std::string resultCategory;
    std::string errorCategory;
    std::string retryClassification;
    std::string boundedDiagnostics;
    std::int64_t completedAt = 0;
};

struct BackendAgentCommandState
{
    BackendAgentCommandEnvelope envelope;
    BackendAgentCommandReceipt receipt;
    BackendAgentCommandResult result;
    bool receiptPresent = false;
    bool resultPresent = false;
    bool resultAcknowledged = false;
    BackendAgentCommandDispatchState dispatchState =
        BackendAgentCommandDispatchState::NotStarted;
};

struct BackendAgentCommandDecision
{
    bool accepted = false;
    bool duplicate = false;
    bool conflict = false;
    bool stale = false;
    bool expired = false;
    std::string reasonCode;
    BackendAgentCommandState state;
};

bool backendAgentValidCommandEnvelope(
    const BackendAgentCommandEnvelope& envelope,
    std::string& reasonCode);

class BackendAgentCommandRepository
{
public:
    explicit BackendAgentCommandRepository(Database& database);

    bool ensureSchema();
    BackendAgentCommandDecision createAssignment(
        const BackendAgentCommandEnvelope& envelope);
    std::optional<BackendAgentCommandEnvelope> nextAssignment(
        const std::string& backendId,
        const std::string& agentId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        std::int64_t now) const;
    BackendAgentCommandDecision recordReceipt(
        const BackendAgentCommandReceipt& receipt);
    BackendAgentCommandDecision recordResult(
        const BackendAgentCommandResult& result);
    bool acknowledgeResult(
        const std::string& commandId,
        const std::string& requestFingerprint,
        std::int64_t acknowledgedAt);
    std::optional<BackendAgentCommandState> find(
        const std::string& commandId) const;

private:
    Database& database_;
};

class BackendAgentLocalCommandStore
{
public:
    explicit BackendAgentLocalCommandStore(std::string stateDirectory);

    bool ensurePrivateStateDirectory() const;
    BackendAgentCommandDecision accept(
        const BackendAgentCommandEnvelope& envelope,
        const std::string& currentAgentId,
        const std::string& currentAgentInstanceId,
        std::uint64_t currentBackendGeneration,
        std::int64_t now);
    bool markStarting(const std::string& commandId);
    bool markAcceptedByExecutor(const std::string& commandId);
    bool persistResult(const BackendAgentCommandResult& result);
    std::vector<BackendAgentCommandResult> pendingResults() const;
    bool acknowledgeResult(
        const std::string& commandId,
        const std::string& requestFingerprint);
    std::optional<BackendAgentCommandState> find(
        const std::string& commandId) const;

private:
    bool writeState(const BackendAgentCommandState& state) const;
    std::optional<BackendAgentCommandState> readState(
        const std::string& commandId) const;
    std::string commandPath(const std::string& commandId) const;

    std::string stateDirectory_;
};

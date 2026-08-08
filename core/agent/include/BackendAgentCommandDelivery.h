#pragma once

#include "BackendAgentCommand.h"
#include "SecurityIdentity.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class AccountabilityEventRepository;
class BackendAgentRepository;
class Database;

struct BackendAgentLocalProviderOwnershipStatus
{
    bool present = false;
    bool active = false;
    vdrsuite::agent::BackendAgentLocalProviderOwnership ownership;
};

class BackendAgentCommandRepository
{
public:
    explicit BackendAgentCommandRepository(Database& database);
    bool ensureSchema();
    bool insertAssignment(
        const BackendAgentCommandAssignment& assignment,
        const vdrsuite::agent::BackendAgentLocalProviderSelection* selection = nullptr);
    bool hasCapability(
        const std::string& backendId,
        const std::string& agentId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        const std::string& commandType) const;
    std::optional<vdrsuite::agent::BackendAgentLocalProviderSelection>
    selectLocalProvider(
        const std::string& backendId,
        const std::string& agentId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        const std::string& authorityDomain,
        const std::string& requiredCapability,
        std::string& reasonCode) const;
    bool localProviderSelectionCurrent(
        const std::string& commandId,
        std::string& reasonCode) const;
    bool setLocalProviderOwnership(
        const std::string& backendId,
        const std::string& authorityDomain,
        const std::string& providerId,
        const std::string& providerKind,
        const std::vector<std::string>& allowedCapabilities,
        std::int64_t updatedAt,
        vdrsuite::agent::BackendAgentLocalProviderOwnership& ownership,
        std::string& reasonCode);
    bool clearLocalProviderOwnership(
        const std::string& backendId,
        const std::string& authorityDomain,
        std::int64_t updatedAt,
        std::string& reasonCode);
    BackendAgentLocalProviderOwnershipStatus localProviderOwnershipStatus(
        const std::string& backendId,
        const std::string& authorityDomain) const;
    BackendAgentCommandPollResult poll(
        const BackendAgentCommandPollRequest& request,
        const std::string& agentId,
        std::int64_t now);
    BackendAgentCommandReceiptResult acceptReceipt(
        const BackendAgentCommandReceipt& receipt);
    BackendAgentCommandResultAck acceptResult(
        const BackendAgentCommandResult& result);
    bool requestReplay(const std::string& backendId, const std::string& commandId);
    bool armFault(const std::string& backendId, const std::string& kind);
    bool consumeFault(const std::string& backendId, const std::string& kind);
    BackendAgentCommandSummary summaryForBackend(const std::string& backendId) const;
private:
    Database& database_;
};

class BackendAgentCommandDeliveryService
{
public:
    BackendAgentCommandDeliveryService(
        BackendAgentCommandRepository& commandRepository,
        BackendAgentRepository& agentRepository,
        AccountabilityEventRepository& accountabilityRepository);

    BackendAgentCommandPollResult poll(
        const RequestSecurityContext& context,
        const BackendAgentCommandPollRequest& request,
        std::int64_t now);
    BackendAgentCommandReceiptResult receipt(
        const RequestSecurityContext& context,
        const BackendAgentCommandReceipt& receipt,
        std::int64_t now);
    BackendAgentCommandResultAck result(
        const RequestSecurityContext& context,
        const BackendAgentCommandResult& result,
        std::int64_t now);

    std::optional<BackendAgentCommandAssignment> assignProbe(
        const RequestSecurityContext& context,
        const std::string& backendId,
        std::int64_t now,
        std::int64_t deadline,
        std::string& reasonCode);
    std::optional<BackendAgentCommandAssignment> assignNativeProbe(
        const RequestSecurityContext& context,
        const std::string& backendId,
        std::int64_t now,
        std::int64_t deadline,
        std::string& reasonCode);
    bool requestReplay(
        const RequestSecurityContext& context,
        const std::string& backendId,
        const std::string& commandId,
        std::int64_t now,
        std::string& reasonCode);
    bool armFault(
        const RequestSecurityContext& context,
        const std::string& backendId,
        const std::string& kind,
        std::int64_t now,
        std::string& reasonCode);
    BackendAgentCommandSummary summaryForBackend(const std::string& backendId) const;

private:
    bool agentContextMatches(
        const RequestSecurityContext& context,
        const std::string& backendId,
        const std::string& agentId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        bool requireLease,
        std::int64_t now,
        std::string& reasonCode) const;
    bool appendEvent(
        const RequestSecurityContext& context,
        const std::string& eventType,
        const std::string& backendId,
        const std::string& operationId,
        const std::string& action,
        const std::string& decision,
        const std::string& reasonCode,
        const std::string& outcome,
        std::int64_t now) const;

    BackendAgentCommandRepository& commandRepository_;
    BackendAgentRepository& agentRepository_;
    AccountabilityEventRepository& accountabilityRepository_;
};

#pragma once

#include "BackendAgentLocalProvider.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::agent
{

struct BackendAgentProtectedWriteResourceRef
{
    std::string resourceType;
    std::string resourceId;
    std::string expectedRevision;
};

struct BackendAgentProtectedWriteResourceScope
{
    std::string resourceType;
    std::string resourceId;
};

struct BackendAgentProtectedWriteObservedResource
{
    std::string resourceType;
    std::string resourceId;
    std::string currentRevision;
};

struct BackendAgentProtectedWriteLease
{
    std::string leaseId;
    std::string backendId;
    std::string authorityDomain;
    std::uint64_t authorityGeneration = 0;
    std::int64_t expiresAt = 0;
    bool active = false;
    std::vector<std::string> allowedCapabilities;
    std::vector<BackendAgentProtectedWriteResourceScope> resources;
};

enum class BackendAgentProtectedWriteIdempotencyState
{
    absent,
    reserved,
    outcomeUnknown,
    completed,
};

struct BackendAgentProtectedWriteIdempotencyRecord
{
    bool present = false;
    std::string idempotencyKey;
    std::string operationId;
    std::string requestFingerprint;
    BackendAgentProtectedWriteIdempotencyState state =
        BackendAgentProtectedWriteIdempotencyState::absent;
    std::string durableResultReference;
};

struct BackendAgentProtectedWriteRequest
{
    std::string operationId;
    std::string idempotencyKey;
    std::string requestFingerprint;
    std::uint64_t expectedAuthorityGeneration = 0;
    std::string leaseId;
    std::int64_t leaseExpiresAt = 0;
    std::string requiredWriteCapability;
    std::vector<BackendAgentProtectedWriteResourceRef> resources;
    BackendAgentLocalProviderSelection providerSelection;
    bool readbackRequired = true;
};

enum class BackendAgentProtectedWriteDecision
{
    reject,
    reserveThenExecuteOnce,
    returnPersistedResult,
    reconcileUnknownOutcome,
};

struct BackendAgentProtectedWriteDecisionResult
{
    BackendAgentProtectedWriteDecision decision =
        BackendAgentProtectedWriteDecision::reject;
    std::string reasonCode;
};

bool backendAgentProtectedWriteValidRequest(
    const BackendAgentProtectedWriteRequest& request);
bool backendAgentProtectedWriteValidLease(
    const BackendAgentProtectedWriteLease& lease);
bool backendAgentProtectedWriteValidIdempotencyRecord(
    const BackendAgentProtectedWriteIdempotencyRecord& record);
bool backendAgentProtectedWriteValidObservedResources(
    const std::vector<BackendAgentProtectedWriteObservedResource>& resources);

BackendAgentProtectedWriteDecisionResult backendAgentProtectedWriteDecide(
    const BackendAgentProtectedWriteRequest& request,
    const BackendAgentProtectedWriteIdempotencyRecord& idempotency,
    const BackendAgentProtectedWriteLease& lease,
    const BackendAgentLocalProviderOwnership& ownership,
    const BackendAgentLocalProviderFacts& facts,
    const std::vector<BackendAgentProtectedWriteObservedResource>& observedResources,
    std::int64_t now);

}

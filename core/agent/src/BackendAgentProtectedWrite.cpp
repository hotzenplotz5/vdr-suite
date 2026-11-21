#include "BackendAgentProtectedWrite.h"

#include <algorithm>
#include <cctype>

namespace vdrsuite::agent
{
namespace
{
bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' ||
            character == '_' || character == '.' || character == ':';
    });
}

bool safeOpaque(const std::string& value, std::size_t maximum = 512)
{
    if (value.empty() || value.size() > maximum) return false;
    return std::none_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U || character == 0x7fU;
    });
}

bool validResourceRef(const BackendAgentProtectedWriteResourceRef& resource)
{
    return safeIdentifier(resource.resourceType) &&
        safeOpaque(resource.resourceId) && safeOpaque(resource.expectedRevision, 256);
}

bool validResourceScope(const BackendAgentProtectedWriteResourceScope& resource)
{
    return safeIdentifier(resource.resourceType) && safeOpaque(resource.resourceId);
}

bool validObservedResource(const BackendAgentProtectedWriteObservedResource& resource)
{
    return safeIdentifier(resource.resourceType) &&
        safeOpaque(resource.resourceId) && safeOpaque(resource.currentRevision, 256);
}

template <typename T, typename Validator>
bool boundedUniqueResources(const std::vector<T>& values, Validator validator)
{
    if (values.empty() || values.size() > 64) return false;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (!validator(values[index])) return false;
        for (std::size_t previous = 0; previous < index; ++previous)
        {
            if (values[index].resourceType == values[previous].resourceType &&
                values[index].resourceId == values[previous].resourceId)
            {
                return false;
            }
        }
    }
    return true;
}

bool boundedUniqueCapabilities(const std::vector<std::string>& capabilities)
{
    if (capabilities.empty() || capabilities.size() > 64) return false;
    for (std::size_t index = 0; index < capabilities.size(); ++index)
    {
        if (!safeIdentifier(capabilities[index])) return false;
        if (std::find(capabilities.begin(), capabilities.begin() + index,
                      capabilities[index]) != capabilities.begin() + index)
        {
            return false;
        }
    }
    return true;
}

bool containsCapability(
    const std::vector<std::string>& capabilities,
    const std::string& capability)
{
    return std::find(capabilities.begin(), capabilities.end(), capability) !=
        capabilities.end();
}

bool scopeContains(
    const std::vector<BackendAgentProtectedWriteResourceScope>& scopes,
    const BackendAgentProtectedWriteResourceRef& resource)
{
    return std::any_of(scopes.begin(), scopes.end(), [&](const auto& scope) {
        return scope.resourceType == resource.resourceType &&
            scope.resourceId == resource.resourceId;
    });
}

const BackendAgentProtectedWriteObservedResource* findObserved(
    const std::vector<BackendAgentProtectedWriteObservedResource>& observed,
    const BackendAgentProtectedWriteResourceRef& resource)
{
    const auto iterator = std::find_if(observed.begin(), observed.end(),
        [&](const auto& current) {
            return current.resourceType == resource.resourceType &&
                current.resourceId == resource.resourceId;
        });
    return iterator == observed.end() ? nullptr : &*iterator;
}

BackendAgentProtectedWriteDecisionResult reject(const std::string& reasonCode)
{
    return {BackendAgentProtectedWriteDecision::reject, reasonCode};
}
}

bool backendAgentProtectedWriteValidRequest(
    const BackendAgentProtectedWriteRequest& request)
{
    return safeIdentifier(request.operationId) &&
        safeOpaque(request.idempotencyKey, 256) &&
        safeOpaque(request.requestFingerprint, 256) &&
        request.expectedAuthorityGeneration > 0 &&
        safeIdentifier(request.leaseId) && request.leaseExpiresAt > 0 &&
        safeIdentifier(request.requiredWriteCapability) &&
        boundedUniqueResources(request.resources, validResourceRef) &&
        backendAgentLocalProviderValidSelection(request.providerSelection) &&
        request.providerSelection.requiredCapability == request.requiredWriteCapability &&
        request.readbackRequired;
}

bool backendAgentProtectedWriteValidLease(
    const BackendAgentProtectedWriteLease& lease)
{
    return safeIdentifier(lease.leaseId) && safeIdentifier(lease.backendId) &&
        safeIdentifier(lease.authorityDomain) && lease.authorityGeneration > 0 &&
        lease.expiresAt > 0 && boundedUniqueCapabilities(lease.allowedCapabilities) &&
        boundedUniqueResources(lease.resources, validResourceScope);
}

bool backendAgentProtectedWriteValidIdempotencyRecord(
    const BackendAgentProtectedWriteIdempotencyRecord& record)
{
    if (!record.present)
    {
        return record.idempotencyKey.empty() && record.operationId.empty() &&
            record.requestFingerprint.empty() && record.durableResultReference.empty() &&
            record.state == BackendAgentProtectedWriteIdempotencyState::absent;
    }
    if (!safeOpaque(record.idempotencyKey, 256) ||
        !safeIdentifier(record.operationId) ||
        !safeOpaque(record.requestFingerprint, 256) ||
        record.state == BackendAgentProtectedWriteIdempotencyState::absent)
    {
        return false;
    }
    if (record.state == BackendAgentProtectedWriteIdempotencyState::completed)
        return safeOpaque(record.durableResultReference);
    return record.durableResultReference.empty();
}

bool backendAgentProtectedWriteValidObservedResources(
    const std::vector<BackendAgentProtectedWriteObservedResource>& resources)
{
    return boundedUniqueResources(resources, validObservedResource);
}

BackendAgentProtectedWriteDecisionResult backendAgentProtectedWriteDecide(
    const BackendAgentProtectedWriteRequest& request,
    const BackendAgentProtectedWriteIdempotencyRecord& idempotency,
    const BackendAgentProtectedWriteLease& lease,
    const BackendAgentLocalProviderOwnership& ownership,
    const BackendAgentLocalProviderFacts& facts,
    const std::vector<BackendAgentProtectedWriteObservedResource>& observedResources,
    std::int64_t now)
{
    if (!backendAgentProtectedWriteValidRequest(request) ||
        !backendAgentProtectedWriteValidIdempotencyRecord(idempotency) || now <= 0)
    {
        return reject("invalid_protected_write_contract");
    }

    // ADR-0042 ordering starts with durable identity/idempotency. Exact completed
    // duplicates and unknown outcomes must never cross a fresh native-write gate.
    if (idempotency.present)
    {
        if (idempotency.idempotencyKey != request.idempotencyKey)
            return reject("protected_write_idempotency_lookup_mismatch");
        if (idempotency.operationId != request.operationId ||
            idempotency.requestFingerprint != request.requestFingerprint)
        {
            return reject("protected_write_idempotency_conflict");
        }
        if (idempotency.state == BackendAgentProtectedWriteIdempotencyState::completed)
        {
            return {BackendAgentProtectedWriteDecision::returnPersistedResult,
                    "protected_write_duplicate_completed"};
        }
        return {BackendAgentProtectedWriteDecision::reconcileUnknownOutcome,
                "protected_write_outcome_requires_reconciliation"};
    }

    // A first execution candidate must cross a fresh, exact, resource-scoped lease.
    if (!backendAgentProtectedWriteValidLease(lease))
        return reject("invalid_protected_write_lease");
    if (!lease.active) return reject("protected_write_lease_inactive");
    if (request.leaseId != lease.leaseId || request.leaseExpiresAt != lease.expiresAt)
        return reject("protected_write_lease_changed");
    if (now >= lease.expiresAt) return reject("protected_write_lease_expired");
    if (lease.backendId != request.providerSelection.backendId ||
        lease.authorityDomain != request.providerSelection.authorityDomain)
    {
        return reject("protected_write_lease_authority_scope_mismatch");
    }
    if (!containsCapability(lease.allowedCapabilities, request.requiredWriteCapability))
        return reject("protected_write_lease_capability_not_authorized");
    for (const auto& resource : request.resources)
    {
        if (!scopeContains(lease.resources, resource))
            return reject("protected_write_lease_resource_not_authorized");
    }

    // Authority generation is independent from resource revision and must match
    // both the fresh lease and the immutable local-provider ownership fence.
    if (request.expectedAuthorityGeneration != lease.authorityGeneration ||
        request.expectedAuthorityGeneration != request.providerSelection.ownershipGeneration)
    {
        return reject("protected_write_authority_generation_changed");
    }
    if (!backendAgentLocalProviderValidOwnership(ownership) ||
        !backendAgentLocalProviderValidFacts(facts))
    {
        return reject("invalid_protected_write_provider_state");
    }
    std::string providerReason;
    if (!backendAgentLocalProviderSelectionUsable(
            request.providerSelection, ownership, facts, providerReason))
    {
        return reject("protected_write_provider_selection_not_current");
    }

    // Revision checks are the final pre-write fence. The observed set must be an
    // exact snapshot of every resource named by the immutable request.
    if (!backendAgentProtectedWriteValidObservedResources(observedResources) ||
        observedResources.size() != request.resources.size())
    {
        return reject("protected_write_resource_state_missing");
    }
    for (const auto& resource : request.resources)
    {
        const auto* observed = findObserved(observedResources, resource);
        if (observed == nullptr)
            return reject("protected_write_resource_state_missing");
        if (observed->currentRevision != resource.expectedRevision)
            return reject("protected_write_revision_conflict");
    }

    // The caller must durably reserve the idempotency key before any native write.
    return {BackendAgentProtectedWriteDecision::reserveThenExecuteOnce,
            "protected_write_authorized_once"};
}

}

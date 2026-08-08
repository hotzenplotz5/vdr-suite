#include "BackendAgentProtectedWrite.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

using namespace vdrsuite::agent;

namespace
{
constexpr std::int64_t Now = 1000;

BackendAgentLocalProviderFacts facts()
{
    BackendAgentLocalProviderFacts value;
    value.providerId = "suitebridge:local";
    value.providerKind = "suitebridge";
    value.providerInstanceEpoch = "pie_contract_1";
    value.providerGeneration = 4;
    value.capabilityRevision = 9;
    value.available = true;
    value.capabilities = {"contract.protected.write"};
    return value;
}

BackendAgentLocalProviderOwnership ownership()
{
    BackendAgentLocalProviderOwnership value;
    value.backendId = "default";
    value.authorityDomain = "contract.protected";
    value.providerId = "suitebridge:local";
    value.providerKind = "suitebridge";
    value.ownershipGeneration = 12;
    value.allowedCapabilities = {"contract.protected.write"};
    return value;
}

BackendAgentProtectedWriteResourceRef resource()
{
    return {"contract.resource", "resource-1", "revision-7"};
}

BackendAgentProtectedWriteLease lease()
{
    BackendAgentProtectedWriteLease value;
    value.leaseId = "lease_contract_1";
    value.backendId = "default";
    value.authorityDomain = "contract.protected";
    value.authorityGeneration = 12;
    value.expiresAt = Now + 60;
    value.active = true;
    value.allowedCapabilities = {"contract.protected.write"};
    value.resources = {{"contract.resource", "resource-1"}};
    return value;
}

BackendAgentProtectedWriteRequest request()
{
    std::string reason;
    BackendAgentProtectedWriteRequest value;
    value.operationId = "operation_contract_1";
    value.idempotencyKey = "idempotency-contract-1";
    value.requestFingerprint = "fingerprint-contract-1";
    value.expectedAuthorityGeneration = 12;
    value.leaseId = "lease_contract_1";
    value.leaseExpiresAt = Now + 60;
    value.requiredWriteCapability = "contract.protected.write";
    value.resources = {resource()};
    value.providerSelection = backendAgentLocalProviderSelect(
        ownership(), facts(), value.requiredWriteCapability, reason);
    assert(reason == "local_provider_selected");
    value.readbackRequired = true;
    return value;
}

std::vector<BackendAgentProtectedWriteObservedResource> observed()
{
    return {{"contract.resource", "resource-1", "revision-7"}};
}

BackendAgentProtectedWriteIdempotencyRecord absentIdempotency()
{
    return {};
}

BackendAgentProtectedWriteIdempotencyRecord completedIdempotency()
{
    const auto value = request();
    BackendAgentProtectedWriteIdempotencyRecord record;
    record.present = true;
    record.idempotencyKey = value.idempotencyKey;
    record.operationId = value.operationId;
    record.requestFingerprint = value.requestFingerprint;
    record.state = BackendAgentProtectedWriteIdempotencyState::completed;
    record.durableResultReference = "result-contract-1";
    return record;
}

BackendAgentProtectedWriteIdempotencyRecord unknownIdempotency()
{
    auto record = completedIdempotency();
    record.state = BackendAgentProtectedWriteIdempotencyState::outcomeUnknown;
    record.durableResultReference.clear();
    return record;
}

void assertDecision(
    const BackendAgentProtectedWriteDecisionResult& result,
    BackendAgentProtectedWriteDecision expected,
    const std::string& reason)
{
    assert(result.decision == expected);
    assert(result.reasonCode == reason);
}
}

int main()
{
    const auto baseRequest = request();
    const auto baseLease = lease();
    const auto baseOwnership = ownership();
    const auto baseFacts = facts();
    const auto baseObserved = observed();

    assert(backendAgentProtectedWriteValidRequest(baseRequest));
    assert(backendAgentProtectedWriteValidLease(baseLease));
    assert(backendAgentProtectedWriteValidIdempotencyRecord(absentIdempotency()));
    assert(backendAgentProtectedWriteValidObservedResources(baseObserved));

    // A fresh write is only a candidate for durable reservation before one execution.
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, absentIdempotency(), baseLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reserveThenExecuteOnce,
        "protected_write_authorized_once");

    // Exact completed duplicates return durable evidence without re-crossing a
    // now-stale write lease/provider fence because they authorize no new write.
    auto expiredLease = baseLease;
    expiredLease.expiresAt = Now - 1;
    auto restartedFacts = baseFacts;
    restartedFacts.providerInstanceEpoch = "pie_contract_2";
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, completedIdempotency(), expiredLease, baseOwnership,
            restartedFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::returnPersistedResult,
        "protected_write_duplicate_completed");

    // A reserved or unknown exact operation is reconciliation-only: never blind retry.
    auto reserved = unknownIdempotency();
    reserved.state = BackendAgentProtectedWriteIdempotencyState::reserved;
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, reserved, expiredLease, baseOwnership,
            restartedFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reconcileUnknownOutcome,
        "protected_write_outcome_requires_reconciliation");
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, unknownIdempotency(), expiredLease, baseOwnership,
            restartedFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reconcileUnknownOutcome,
        "protected_write_outcome_requires_reconciliation");

    // Same idempotency key with a changed logical request fails before lease checks.
    auto changedRequest = baseRequest;
    changedRequest.requestFingerprint = "fingerprint-contract-changed";
    assertDecision(
        backendAgentProtectedWriteDecide(
            changedRequest, completedIdempotency(), expiredLease, baseOwnership,
            restartedFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_idempotency_conflict");

    auto changedOperation = baseRequest;
    changedOperation.operationId = "operation_contract_2";
    assertDecision(
        backendAgentProtectedWriteDecide(
            changedOperation, completedIdempotency(), baseLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_idempotency_conflict");

    auto wrongLookup = completedIdempotency();
    wrongLookup.idempotencyKey = "other-idempotency-key";
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, wrongLookup, baseLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_idempotency_lookup_mismatch");

    // Lease validity and exact scope precede authority/revision checks.
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, absentIdempotency(), expiredLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_lease_changed");

    auto expiredMatchingRequest = baseRequest;
    expiredMatchingRequest.leaseExpiresAt = expiredLease.expiresAt;
    assertDecision(
        backendAgentProtectedWriteDecide(
            expiredMatchingRequest, absentIdempotency(), expiredLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_lease_expired");

    auto inactiveLease = baseLease;
    inactiveLease.active = false;
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, absentIdempotency(), inactiveLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_lease_inactive");

    auto capabilityLease = baseLease;
    capabilityLease.allowedCapabilities = {"contract.read"};
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, absentIdempotency(), capabilityLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_lease_capability_not_authorized");

    auto resourceLease = baseLease;
    resourceLease.resources = {{"contract.resource", "resource-2"}};
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, absentIdempotency(), resourceLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_lease_resource_not_authorized");

    // Authority generation is distinct from resource revision and wins first.
    auto staleAuthorityRequest = baseRequest;
    ++staleAuthorityRequest.expectedAuthorityGeneration;
    auto staleRevision = baseObserved;
    staleRevision[0].currentRevision = "revision-8";
    assertDecision(
        backendAgentProtectedWriteDecide(
            staleAuthorityRequest, absentIdempotency(), baseLease, baseOwnership,
            baseFacts, staleRevision, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_authority_generation_changed");

    auto newOwnership = baseOwnership;
    ++newOwnership.ownershipGeneration;
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, absentIdempotency(), baseLease, newOwnership,
            baseFacts, staleRevision, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_provider_selection_not_current");

    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, absentIdempotency(), baseLease, baseOwnership,
            baseFacts, staleRevision, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_revision_conflict");

    std::vector<BackendAgentProtectedWriteObservedResource> missingObserved;
    assertDecision(
        backendAgentProtectedWriteDecide(
            baseRequest, absentIdempotency(), baseLease, baseOwnership,
            baseFacts, missingObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "protected_write_resource_state_missing");

    // Protected writes always demand authoritative post-write readback.
    auto noReadback = baseRequest;
    noReadback.readbackRequired = false;
    assertDecision(
        backendAgentProtectedWriteDecide(
            noReadback, absentIdempotency(), baseLease, baseOwnership,
            baseFacts, baseObserved, Now),
        BackendAgentProtectedWriteDecision::reject,
        "invalid_protected_write_contract");

    // The write capability is part of the immutable provider selection.
    auto mismatchedCapability = baseRequest;
    mismatchedCapability.requiredWriteCapability = "contract.other.write";
    assert(!backendAgentProtectedWriteValidRequest(mismatchedCapability));

    // Completed evidence is only valid when a durable result reference exists.
    auto incompleteCompleted = completedIdempotency();
    incompleteCompleted.durableResultReference.clear();
    assert(!backendAgentProtectedWriteValidIdempotencyRecord(incompleteCompleted));

    return 0;
}

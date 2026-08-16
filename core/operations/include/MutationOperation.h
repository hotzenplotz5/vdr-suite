#pragma once

#include <cstdint>
#include <string>

namespace vdrsuite::operations
{

enum class MutationOperationState
{
    accepted,
    rejected,
    conflict,
    queued,
    dispatching,
    executedUnverified,
    succeeded,
    failedBeforeDispatch,
    failedVerified,
    outcomeUnknown,
    cancelled,
};

enum class MutationOperationVerificationPolicy
{
    none,
    readbackRequired,
    eventConfirmation,
    reconciliationRequired,
};

struct MutationOperation
{
    std::string operationId;
    std::string idempotencyKey;
    std::string actorId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string resourceType;
    std::string resourceId;
    std::string expectedRevision;
    std::string expectedResourceFingerprint;
    std::string actionFamily;
    std::string requestFingerprint;
    std::int64_t requestedAt = 0;
    std::int64_t deadline = 0;
    MutationOperationVerificationPolicy verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    MutationOperationState state = MutationOperationState::accepted;
    std::string resultReference;
    std::int64_t updatedAt = 0;
    std::string operationRevision;
};

const char* mutationOperationStateName(MutationOperationState state);
bool mutationOperationStateFromName(
    const std::string& value,
    MutationOperationState& state);
const char* mutationOperationVerificationPolicyName(
    MutationOperationVerificationPolicy policy);
bool mutationOperationVerificationPolicyFromName(
    const std::string& value,
    MutationOperationVerificationPolicy& policy);
bool mutationOperationStateTerminal(MutationOperationState state);
bool mutationOperationTransitionAllowed(
    MutationOperationState from,
    MutationOperationState to);
bool mutationOperationValidForCreate(const MutationOperation& operation);
bool mutationOperationValidDurable(const MutationOperation& operation);

}
#include "MutationOperation.h"

#include <limits>

namespace vdrsuite::operations
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxFingerprintLength = 512;
constexpr std::size_t kMaxResultReferenceLength = 512;

bool safeIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool validRevisionToken(const std::string& value)
{
    return safeIdentity(value);
}

bool parseOperationRevision(const std::string& value)
{
    if (value.empty()) return false;
    std::uint64_t parsed = 0;
    for (char ch : value)
    {
        if (ch < '0' || ch > '9') return false;
        const std::uint64_t digit = static_cast<std::uint64_t>(ch - '0');
        if (parsed > (static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max()) - digit) / 10)
            return false;
        parsed = parsed * 10 + digit;
    }
    return parsed > 0;
}

bool commonValid(const MutationOperation& operation)
{
    if (!safeIdentity(operation.operationId) ||
        !safeIdentity(operation.idempotencyKey) ||
        !safeIdentity(operation.actorId) ||
        !safeIdentity(operation.backendId) ||
        operation.backendGeneration == 0 ||
        operation.backendGeneration > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        !safeIdentity(operation.resourceType) ||
        !safeIdentity(operation.resourceId) ||
        !validRevisionToken(operation.expectedRevision) ||
        !safeIdentity(operation.actionFamily) ||
        operation.requestFingerprint.empty() ||
        operation.requestFingerprint.size() > kMaxFingerprintLength ||
        operation.requestedAt <= 0 ||
        operation.updatedAt < operation.requestedAt ||
        (operation.deadline != 0 && operation.deadline < operation.requestedAt) ||
        operation.resultReference.size() > kMaxResultReferenceLength)
        return false;

    switch (operation.verificationPolicy)
    {
        case MutationOperationVerificationPolicy::none:
        case MutationOperationVerificationPolicy::readbackRequired:
        case MutationOperationVerificationPolicy::eventConfirmation:
        case MutationOperationVerificationPolicy::reconciliationRequired:
            break;
        default:
            return false;
    }

    switch (operation.state)
    {
        case MutationOperationState::accepted:
        case MutationOperationState::rejected:
        case MutationOperationState::conflict:
        case MutationOperationState::queued:
        case MutationOperationState::dispatching:
        case MutationOperationState::executedUnverified:
        case MutationOperationState::succeeded:
        case MutationOperationState::failedBeforeDispatch:
        case MutationOperationState::failedVerified:
        case MutationOperationState::outcomeUnknown:
        case MutationOperationState::cancelled:
            return true;
        default:
            return false;
    }
}
}

const char* mutationOperationStateName(MutationOperationState state)
{
    switch (state)
    {
        case MutationOperationState::accepted: return "accepted";
        case MutationOperationState::rejected: return "rejected";
        case MutationOperationState::conflict: return "conflict";
        case MutationOperationState::queued: return "queued";
        case MutationOperationState::dispatching: return "dispatching";
        case MutationOperationState::executedUnverified: return "executed_unverified";
        case MutationOperationState::succeeded: return "succeeded";
        case MutationOperationState::failedBeforeDispatch: return "failed_before_dispatch";
        case MutationOperationState::failedVerified: return "failed_verified";
        case MutationOperationState::outcomeUnknown: return "outcome_unknown";
        case MutationOperationState::cancelled: return "cancelled";
    }
    return "invalid";
}

bool mutationOperationStateFromName(
    const std::string& value,
    MutationOperationState& state)
{
    if (value == "accepted") state = MutationOperationState::accepted;
    else if (value == "rejected") state = MutationOperationState::rejected;
    else if (value == "conflict") state = MutationOperationState::conflict;
    else if (value == "queued") state = MutationOperationState::queued;
    else if (value == "dispatching") state = MutationOperationState::dispatching;
    else if (value == "executed_unverified") state = MutationOperationState::executedUnverified;
    else if (value == "succeeded") state = MutationOperationState::succeeded;
    else if (value == "failed_before_dispatch") state = MutationOperationState::failedBeforeDispatch;
    else if (value == "failed_verified") state = MutationOperationState::failedVerified;
    else if (value == "outcome_unknown") state = MutationOperationState::outcomeUnknown;
    else if (value == "cancelled") state = MutationOperationState::cancelled;
    else return false;
    return true;
}

const char* mutationOperationVerificationPolicyName(
    MutationOperationVerificationPolicy policy)
{
    switch (policy)
    {
        case MutationOperationVerificationPolicy::none: return "none";
        case MutationOperationVerificationPolicy::readbackRequired: return "readback_required";
        case MutationOperationVerificationPolicy::eventConfirmation: return "event_confirmation";
        case MutationOperationVerificationPolicy::reconciliationRequired: return "reconciliation_required";
    }
    return "invalid";
}

bool mutationOperationVerificationPolicyFromName(
    const std::string& value,
    MutationOperationVerificationPolicy& policy)
{
    if (value == "none") policy = MutationOperationVerificationPolicy::none;
    else if (value == "readback_required") policy = MutationOperationVerificationPolicy::readbackRequired;
    else if (value == "event_confirmation") policy = MutationOperationVerificationPolicy::eventConfirmation;
    else if (value == "reconciliation_required") policy = MutationOperationVerificationPolicy::reconciliationRequired;
    else return false;
    return true;
}

bool mutationOperationStateTerminal(MutationOperationState state)
{
    return state == MutationOperationState::rejected ||
        state == MutationOperationState::conflict ||
        state == MutationOperationState::succeeded ||
        state == MutationOperationState::failedBeforeDispatch ||
        state == MutationOperationState::failedVerified ||
        state == MutationOperationState::cancelled;
}

bool mutationOperationTransitionAllowed(
    MutationOperationState from,
    MutationOperationState to)
{
    if (from == to) return true;
    switch (from)
    {
        case MutationOperationState::accepted:
            return to == MutationOperationState::rejected ||
                to == MutationOperationState::conflict ||
                to == MutationOperationState::queued ||
                to == MutationOperationState::dispatching ||
                to == MutationOperationState::failedBeforeDispatch ||
                to == MutationOperationState::cancelled;
        case MutationOperationState::queued:
            return to == MutationOperationState::dispatching ||
                to == MutationOperationState::failedBeforeDispatch ||
                to == MutationOperationState::cancelled;
        case MutationOperationState::dispatching:
            return to == MutationOperationState::executedUnverified ||
                to == MutationOperationState::outcomeUnknown ||
                to == MutationOperationState::failedVerified;
        case MutationOperationState::executedUnverified:
            return to == MutationOperationState::succeeded ||
                to == MutationOperationState::failedVerified ||
                to == MutationOperationState::outcomeUnknown;
        case MutationOperationState::outcomeUnknown:
            return to == MutationOperationState::executedUnverified ||
                to == MutationOperationState::succeeded ||
                to == MutationOperationState::failedVerified;
        case MutationOperationState::rejected:
        case MutationOperationState::conflict:
        case MutationOperationState::succeeded:
        case MutationOperationState::failedBeforeDispatch:
        case MutationOperationState::failedVerified:
        case MutationOperationState::cancelled:
            return false;
    }
    return false;
}

bool mutationOperationValidForCreate(const MutationOperation& operation)
{
    return commonValid(operation) &&
        operation.operationRevision.empty() &&
        operation.state == MutationOperationState::accepted &&
        operation.resultReference.empty() &&
        operation.updatedAt == operation.requestedAt;
}

bool mutationOperationValidDurable(const MutationOperation& operation)
{
    return commonValid(operation) && parseOperationRevision(operation.operationRevision);
}

}

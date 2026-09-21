#include "NativeTimerCreateDispatchService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerSpecification.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::timers
{
namespace
{
constexpr const char* kReferencePrefix =
    "native-timer-create-command-reservation/1|";
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxFingerprintLength = 512;
constexpr std::size_t kMaxResourceFingerprintLength = 4096;
constexpr std::size_t kMaxEvidenceReferenceLength = 512;

bool boundedIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

void appendField(std::string& output, const std::string& value)
{
    output += std::to_string(value.size());
    output += ':';
    output += value;
    output += '|';
}

bool readField(const std::string& input, std::size_t& position, std::string& value)
{
    if (position >= input.size()) return false;
    std::size_t length = 0;
    bool digitSeen = false;
    while (position < input.size() && input[position] != ':')
    {
        const char character = input[position++];
        if (character < '0' || character > '9') return false;
        digitSeen = true;
        const std::size_t digit = static_cast<std::size_t>(character - '0');
        if (length > (std::numeric_limits<std::size_t>::max() - digit) / 10)
            return false;
        length = length * 10 + digit;
    }
    if (!digitSeen || position >= input.size() || input[position] != ':') return false;
    ++position;
    if (length > input.size() - position) return false;
    value = input.substr(position, length);
    position += length;
    if (position >= input.size() || input[position] != '|') return false;
    ++position;
    return true;
}

NativeTimerCreateDispatchClaimResult result(
    NativeTimerCreateDispatchClaimStatus status,
    const vdrsuite::operations::MutationOperation& operation = {},
    const NativeTimerCreateCommandReservationReference& reservation = {})
{
    NativeTimerCreateDispatchClaimResult value;
    value.status = status;
    value.operation = operation;
    value.reservation = reservation;
    return value;
}

bool requestValid(const NativeTimerCreateDispatchClaimRequest& request)
{
    return boundedIdentity(request.operationId) &&
        boundedIdentity(request.expectedOperationRevision) &&
        boundedIdentity(request.timerAssignmentId) &&
        boundedIdentity(request.nativeTimerBindingId) &&
        boundedIdentity(request.backendId) && request.backendGeneration > 0 &&
        !request.expectedSpecificationFingerprint.empty() &&
        request.expectedSpecificationFingerprint.size() <=
            kMaxResourceFingerprintLength &&
        nativeTimerCreateCommandReservationReferenceValid(request.reservation);
}
}

bool nativeTimerCreateCommandReservationReferenceValid(
    const NativeTimerCreateCommandReservationReference& reference)
{
    return boundedIdentity(reference.commandId) &&
        !reference.requestFingerprint.empty() &&
        reference.requestFingerprint.size() <= kMaxFingerprintLength;
}

std::string serializeNativeTimerCreateCommandReservationReference(
    const NativeTimerCreateCommandReservationReference& reference)
{
    if (!nativeTimerCreateCommandReservationReferenceValid(reference)) return {};
    std::string serialized(kReferencePrefix);
    appendField(serialized, reference.commandId);
    appendField(serialized, reference.requestFingerprint);
    return serialized.size() <= 512 ? serialized : std::string{};
}

bool parseNativeTimerCreateCommandReservationReference(
    const std::string& serialized,
    NativeTimerCreateCommandReservationReference& reference)
{
    const std::string prefix(kReferencePrefix);
    if (serialized.compare(0, prefix.size(), prefix) != 0 ||
        serialized.size() > 512)
        return false;
    std::size_t position = prefix.size();
    NativeTimerCreateCommandReservationReference candidate;
    if (!readField(serialized, position, candidate.commandId) ||
        !readField(serialized, position, candidate.requestFingerprint) ||
        position != serialized.size() ||
        !nativeTimerCreateCommandReservationReferenceValid(candidate) ||
        serializeNativeTimerCreateCommandReservationReference(candidate) != serialized)
        return false;
    reference = candidate;
    return true;
}

NativeTimerCreateDispatchService::NativeTimerCreateDispatchService(
    vdrsuite::operations::MutationOperationRepository& operationRepository)
    : operationRepository_(operationRepository)
{
}

NativeTimerCreateDispatchClaimResult
NativeTimerCreateDispatchService::claimAfterReservation(
    const NativeTimerCreateDispatchClaimRequest& request,
    std::int64_t claimedAt)
{
    using namespace vdrsuite::operations;
    if (!requestValid(request) || claimedAt <= 0)
        return result(NativeTimerCreateDispatchClaimStatus::invalid);

    const auto found = operationRepository_.findById(request.operationId);
    if (found.status == MutationOperationRepositoryStatus::notFound)
        return result(NativeTimerCreateDispatchClaimStatus::operationNotFound);
    if (!found.ok())
        return result(NativeTimerCreateDispatchClaimStatus::operationRepositoryError);
    const MutationOperation& operation = found.operation;

    const auto durablePayload =
        operationRepository_.findPayloadByOperationId(request.operationId);
    if (durablePayload.status == MutationOperationRepositoryStatus::notFound)
        return result(
            NativeTimerCreateDispatchClaimStatus::payloadNotFound,
            operation);
    if (!durablePayload.ok())
        return result(
            NativeTimerCreateDispatchClaimStatus::operationRepositoryError,
            operation);
    if (durablePayload.payload.payloadType != "native.timer.create" ||
        durablePayload.payload.payloadVersion != 1)
        return result(
            NativeTimerCreateDispatchClaimStatus::payloadConflict,
            operation);

    NativeTimerCreateOperationPayload payload;
    if (!parseNativeTimerCreateOperationPayload(
            durablePayload.payload.payload, payload) ||
        nativeTimerCreateOperationPayloadFingerprint(payload) !=
            durablePayload.payload.payloadFingerprint)
        return result(
            NativeTimerCreateDispatchClaimStatus::payloadConflict,
            operation);

    const std::string specificationFingerprint =
        nativeTimerSpecificationFingerprint(payload.expectedSpecification);
    if (operation.actionFamily != "timer.create" ||
        operation.resourceType != "TimerAssignment" ||
        operation.resourceId != request.timerAssignmentId ||
        operation.backendId != request.backendId ||
        operation.backendGeneration != request.backendGeneration ||
        payload.timerAssignmentId != request.timerAssignmentId ||
        payload.nativeTimerBindingId != request.nativeTimerBindingId ||
        payload.backendId != request.backendId ||
        payload.backendGeneration != request.backendGeneration)
        return result(
            NativeTimerCreateDispatchClaimStatus::identityConflict,
            operation);

    if (specificationFingerprint.empty() ||
        specificationFingerprint != request.expectedSpecificationFingerprint ||
        operation.expectedResourceFingerprint != specificationFingerprint)
        return result(
            NativeTimerCreateDispatchClaimStatus::payloadConflict,
            operation);

    if (operation.verificationPolicy !=
        MutationOperationVerificationPolicy::readbackRequired)
        return result(
            NativeTimerCreateDispatchClaimStatus::operationStateConflict,
            operation);

    if (operation.state != MutationOperationState::accepted &&
        operation.state != MutationOperationState::dispatching)
        return result(
            NativeTimerCreateDispatchClaimStatus::operationStateConflict,
            operation);
    if (operation.state == MutationOperationState::accepted &&
        operation.operationRevision != request.expectedOperationRevision)
        return result(
            NativeTimerCreateDispatchClaimStatus::operationRevisionConflict,
            operation);
    if (operation.state == MutationOperationState::accepted &&
        operation.deadline != 0 && claimedAt > operation.deadline)
        return result(
            NativeTimerCreateDispatchClaimStatus::deadlineExpired,
            operation);

    const std::string reservationReference =
        serializeNativeTimerCreateCommandReservationReference(request.reservation);
    if (reservationReference.empty())
        return result(NativeTimerCreateDispatchClaimStatus::invalid, operation);

    if (operation.state == MutationOperationState::dispatching)
    {
        NativeTimerCreateCommandReservationReference existingReference;
        if (!parseNativeTimerCreateCommandReservationReference(
                operation.resultReference, existingReference) ||
            existingReference.commandId != request.reservation.commandId ||
            existingReference.requestFingerprint !=
                request.reservation.requestFingerprint)
            return result(
                NativeTimerCreateDispatchClaimStatus::identityConflict,
                operation);
        return result(
            NativeTimerCreateDispatchClaimStatus::alreadyClaimed,
            operation,
            existingReference);
    }

    const auto transitioned = operationRepository_.transition(
        request.operationId,
        request.expectedOperationRevision,
        MutationOperationState::accepted,
        MutationOperationState::dispatching,
        reservationReference,
        claimedAt);
    switch (transitioned.status)
    {
        case MutationOperationRepositoryStatus::ok:
            return result(
                NativeTimerCreateDispatchClaimStatus::claimed,
                transitioned.operation,
                request.reservation);
        case MutationOperationRepositoryStatus::idempotentReplay:
            return result(
                NativeTimerCreateDispatchClaimStatus::alreadyClaimed,
                transitioned.operation,
                request.reservation);
        case MutationOperationRepositoryStatus::notFound:
            return result(NativeTimerCreateDispatchClaimStatus::operationNotFound);
        case MutationOperationRepositoryStatus::revisionConflict:
            return result(
                NativeTimerCreateDispatchClaimStatus::operationRevisionConflict,
                transitioned.operation);
        case MutationOperationRepositoryStatus::stateConflict:
            return result(
                NativeTimerCreateDispatchClaimStatus::operationStateConflict,
                transitioned.operation);
        case MutationOperationRepositoryStatus::invalid:
            return result(NativeTimerCreateDispatchClaimStatus::invalid);
        case MutationOperationRepositoryStatus::storageError:
            return result(
                NativeTimerCreateDispatchClaimStatus::operationRepositoryError);
        case MutationOperationRepositoryStatus::idempotencyConflict:
        case MutationOperationRepositoryStatus::operationConflict:
            return result(
                NativeTimerCreateDispatchClaimStatus::operationStateConflict,
                transitioned.operation);
    }
    return result(NativeTimerCreateDispatchClaimStatus::operationRepositoryError);
}


namespace
{
NativeTimerCreateDispatchOutcomeResult outcomeResult(
    NativeTimerCreateDispatchOutcomeStatus status,
    const vdrsuite::operations::MutationOperation& operation = {},
    const NativeTimerCreateReadbackExpectation* expectation = nullptr)
{
    NativeTimerCreateDispatchOutcomeResult value;
    value.status = status;
    value.operation = operation;
    if (expectation != nullptr)
    {
        value.expectationPresent = true;
        value.expectation = *expectation;
    }
    return value;
}

bool outcomeShapeValid(const NativeTimerCreateExecutorOutcome& outcome)
{
    if (!boundedIdentity(outcome.operationId) ||
        !boundedIdentity(outcome.operationRevision) ||
        !nativeTimerCreateCommandReservationReferenceValid(
            outcome.reservation) ||
        outcome.completedAt <= 0 ||
        outcome.evidenceReference.empty() ||
        outcome.evidenceReference.size() > kMaxEvidenceReferenceLength)
        return false;

    switch (outcome.category)
    {
        case NativeTimerCreateExecutorOutcomeCategory::rejectedWithoutEffect:
            return outcome.dispatchStartedAt == 0;
        case NativeTimerCreateExecutorOutcomeCategory::acceptedUnverified:
        case NativeTimerCreateExecutorOutcomeCategory::outcomeUnknown:
            return outcome.dispatchStartedAt > 0 &&
                outcome.dispatchStartedAt <= outcome.completedAt;
    }
    return false;
}

vdrsuite::operations::MutationOperationState outcomeState(
    NativeTimerCreateExecutorOutcomeCategory category)
{
    using vdrsuite::operations::MutationOperationState;
    switch (category)
    {
        case NativeTimerCreateExecutorOutcomeCategory::rejectedWithoutEffect:
            return MutationOperationState::failedVerified;
        case NativeTimerCreateExecutorOutcomeCategory::acceptedUnverified:
            return MutationOperationState::executedUnverified;
        case NativeTimerCreateExecutorOutcomeCategory::outcomeUnknown:
            return MutationOperationState::outcomeUnknown;
    }
    return MutationOperationState::outcomeUnknown;
}

bool operationMatchesPayload(
    const vdrsuite::operations::MutationOperation& operation,
    const NativeTimerCreateOperationPayload& payload)
{
    const std::string fingerprint =
        nativeTimerSpecificationFingerprint(payload.expectedSpecification);
    return operation.backendId == payload.backendId &&
        operation.backendGeneration == payload.backendGeneration &&
        operation.resourceType == "TimerAssignment" &&
        operation.resourceId == payload.timerAssignmentId &&
        operation.expectedRevision == payload.expectedAssignmentRevision &&
        operation.expectedResourceFingerprint == fingerprint &&
        operation.actionFamily == "timer.create" &&
        operation.verificationPolicy ==
            vdrsuite::operations::MutationOperationVerificationPolicy::
                readbackRequired;
}

NativeTimerCreateReadbackExpectation expectationFor(
    const NativeTimerCreateExecutorOutcome& outcome,
    const NativeTimerCreateOperationPayload& payload)
{
    NativeTimerCreateReadbackExpectation expectation;
    expectation.operationId = outcome.operationId;
    expectation.operationState =
        outcome.category ==
            NativeTimerCreateExecutorOutcomeCategory::acceptedUnverified
        ? NativeTimerReadbackOperationState::executedUnverified
        : NativeTimerReadbackOperationState::outcomeUnknown;
    expectation.timerAssignmentId = payload.timerAssignmentId;
    expectation.nativeTimerBindingId = payload.nativeTimerBindingId;
    expectation.backendId = payload.backendId;
    expectation.backendGeneration = payload.backendGeneration;
    expectation.readbackNotBefore = outcome.dispatchStartedAt;
    expectation.expectedSpecification = payload.expectedSpecification;
    expectation.expectedSpecificationFingerprint =
        nativeTimerSpecificationFingerprint(payload.expectedSpecification);
    return expectation;
}
} // namespace

NativeTimerCreateDispatchOutcomeResult
NativeTimerCreateDispatchService::applyOutcome(
    const NativeTimerCreateExecutorOutcome& outcome)
{
    using namespace vdrsuite::operations;
    if (!outcomeShapeValid(outcome))
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::invalid);

    const auto found = operationRepository_.findById(outcome.operationId);
    if (found.status == MutationOperationRepositoryStatus::notFound)
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::operationNotFound);
    if (!found.ok())
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::
                operationRepositoryError);
    const MutationOperation& operation = found.operation;

    const MutationOperationState target = outcomeState(outcome.category);
    if (operation.state != MutationOperationState::dispatching &&
        operation.state != target)
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::operationStateConflict,
            operation);
    if (operation.state == MutationOperationState::dispatching &&
        operation.operationRevision != outcome.operationRevision)
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::
                operationRevisionConflict,
            operation);
    if (outcome.completedAt < operation.updatedAt)
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::invalid,
            operation);
    if (operation.state == MutationOperationState::dispatching &&
        outcome.category !=
            NativeTimerCreateExecutorOutcomeCategory::rejectedWithoutEffect &&
        outcome.dispatchStartedAt < operation.updatedAt)
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::invalid,
            operation);

    if (operation.state == MutationOperationState::dispatching)
    {
        NativeTimerCreateCommandReservationReference reserved;
        if (!parseNativeTimerCreateCommandReservationReference(
                operation.resultReference, reserved) ||
            reserved.commandId != outcome.reservation.commandId ||
            reserved.requestFingerprint !=
                outcome.reservation.requestFingerprint)
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::identityConflict,
                operation);
    }
    else if (operation.resultReference != outcome.evidenceReference)
    {
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::operationStateConflict,
            operation);
    }

    const auto durable =
        operationRepository_.findPayloadByOperationId(outcome.operationId);
    if (durable.status == MutationOperationRepositoryStatus::notFound)
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::payloadNotFound,
            operation);
    if (!durable.ok())
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::
                operationRepositoryError,
            operation);
    if (durable.payload.payloadType != "native.timer.create" ||
        durable.payload.payloadVersion != 1)
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::payloadConflict,
            operation);

    NativeTimerCreateOperationPayload payload;
    if (!parseNativeTimerCreateOperationPayload(
            durable.payload.payload, payload) ||
        durable.payload.payload !=
            serializeNativeTimerCreateOperationPayload(payload) ||
        durable.payload.payloadFingerprint !=
            nativeTimerCreateOperationPayloadFingerprint(payload))
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::payloadConflict,
            operation);
    if (!operationMatchesPayload(operation, payload))
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::identityConflict,
            operation);

    NativeTimerCreateReadbackExpectation expectation;
    const bool needsReadback =
        outcome.category !=
            NativeTimerCreateExecutorOutcomeCategory::rejectedWithoutEffect;
    if (needsReadback)
    {
        expectation = expectationFor(outcome, payload);
        if (!nativeTimerCreateReadbackExpectationValid(expectation))
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::invalid,
                operation);
    }

    if (operation.state == target)
        return outcomeResult(
            NativeTimerCreateDispatchOutcomeStatus::alreadyApplied,
            operation,
            needsReadback ? &expectation : nullptr);

    const auto transitioned = operationRepository_.transition(
        operation.operationId,
        outcome.operationRevision,
        MutationOperationState::dispatching,
        target,
        outcome.evidenceReference,
        outcome.completedAt);
    switch (transitioned.status)
    {
        case MutationOperationRepositoryStatus::ok:
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::applied,
                transitioned.operation,
                needsReadback ? &expectation : nullptr);
        case MutationOperationRepositoryStatus::idempotentReplay:
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::alreadyApplied,
                transitioned.operation,
                needsReadback ? &expectation : nullptr);
        case MutationOperationRepositoryStatus::notFound:
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::operationNotFound);
        case MutationOperationRepositoryStatus::revisionConflict:
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::
                    operationRevisionConflict,
                transitioned.operation);
        case MutationOperationRepositoryStatus::stateConflict:
        case MutationOperationRepositoryStatus::idempotencyConflict:
        case MutationOperationRepositoryStatus::operationConflict:
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::
                    operationStateConflict,
                transitioned.operation);
        case MutationOperationRepositoryStatus::invalid:
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::invalid,
                transitioned.operation);
        case MutationOperationRepositoryStatus::storageError:
            return outcomeResult(
                NativeTimerCreateDispatchOutcomeStatus::
                    operationRepositoryError,
                transitioned.operation);
    }
    return outcomeResult(
        NativeTimerCreateDispatchOutcomeStatus::operationRepositoryError,
        operation);
}

} // namespace vdrsuite::timers

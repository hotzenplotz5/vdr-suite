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

} // namespace vdrsuite::timers

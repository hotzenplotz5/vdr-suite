#include "NativeTimerCreateReadbackReconciliation.h"

#include "MutationOperationRepository.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerSpecification.h"

namespace vdrsuite::daemon
{
namespace
{
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::timers::NativeTimerCreateOperationPayload;
using Status = NativeTimerCreateReadbackReconciliationStatus;

NativeTimerCreateReadbackReconciliationResult result(
    Status status,
    const std::string& reasonCode,
    const vdrsuite::timers::NativeTimerCreateReadbackVerificationResult&
        verification = {},
    const vdrsuite::timers::TimerAssignmentFulfillmentResult& fulfillment = {},
    const vdrsuite::timers::NativeTimerCreateOperationCompletionResult&
        completion = {})
{
    NativeTimerCreateReadbackReconciliationResult value;
    value.status = status;
    value.reasonCode = reasonCode;
    value.verification = verification;
    value.fulfillment = fulfillment;
    value.completion = completion;
    return value;
}

bool payloadMatchesExpectation(
    const NativeTimerCreateOperationPayload& payload,
    const vdrsuite::timers::NativeTimerCreateReadbackExpectation& expectation)
{
    const std::string fingerprint =
        vdrsuite::timers::nativeTimerSpecificationFingerprint(
            payload.expectedSpecification);

    return payload.timerAssignmentId == expectation.timerAssignmentId &&
        payload.nativeTimerBindingId == expectation.nativeTimerBindingId &&
        payload.backendId == expectation.backendId &&
        payload.backendGeneration == expectation.backendGeneration &&
        fingerprint == expectation.expectedSpecificationFingerprint &&
        fingerprint ==
            vdrsuite::timers::nativeTimerSpecificationFingerprint(
                expectation.expectedSpecification);
}
} // namespace

NativeTimerCreateReadbackReconciliationResult
reconcileNativeTimerCreateReadback(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    vdrsuite::timers::NativeTimerCreateReadbackVerificationService&
        verificationService,
    vdrsuite::timers::TimerAssignmentFulfillmentService& fulfillmentService,
    vdrsuite::timers::NativeTimerCreateOperationCompletionService&
        completionService,
    const vdrsuite::timers::NativeTimerCreateReadbackExpectation& expectation,
    const vdrsuite::timers::NativeTimerCreateReadbackEvidence& readbackEvidence,
    std::int64_t reconciledAt)
{
    if (!vdrsuite::timers::nativeTimerCreateReadbackExpectationValid(
            expectation) ||
        !vdrsuite::timers::nativeTimerCreateReadbackEvidenceValid(
            readbackEvidence))
    {
        return result(Status::readbackVerificationFailed,
            "native_timer_create_readback_reconciliation_invalid_evidence");
    }

    if (reconciledAt <= 0 || reconciledAt < readbackEvidence.observedAt)
    {
        return result(
            Status::staleReconciliationTime,
            "native_timer_create_readback_reconciliation_time_conflict");
    }

    const auto durable =
        operationRepository.findPayloadByOperationId(expectation.operationId);
    if (durable.status == MutationOperationRepositoryStatus::notFound)
    {
        return result(
            Status::payloadNotFound,
            "native_timer_create_operation_payload_not_found");
    }
    if (!durable.ok())
    {
        return result(
            Status::payloadInvalid,
            "native_timer_create_operation_payload_lookup_failed");
    }
    if (durable.payload.payloadType != "native.timer.create" ||
        durable.payload.payloadVersion != 1)
    {
        return result(
            Status::payloadInvalid,
            "native_timer_create_operation_payload_type_conflict");
    }

    NativeTimerCreateOperationPayload payload;
    if (!vdrsuite::timers::parseNativeTimerCreateOperationPayload(
            durable.payload.payload, payload) ||
        durable.payload.payload !=
            vdrsuite::timers::serializeNativeTimerCreateOperationPayload(payload) ||
        durable.payload.payloadFingerprint !=
            vdrsuite::timers::nativeTimerCreateOperationPayloadFingerprint(payload))
    {
        return result(
            Status::payloadInvalid,
            "native_timer_create_operation_payload_invalid");
    }

    if (!payloadMatchesExpectation(payload, expectation))
    {
        return result(
            Status::identityConflict,
            "native_timer_create_readback_expectation_payload_conflict");
    }

    const auto verified =
        verificationService.verify(expectation, readbackEvidence);
    if (!verified.ok())
    {
        return result(
            Status::readbackVerificationFailed,
            "native_timer_create_readback_verification_failed",
            verified);
    }

    const auto fulfilled = fulfillmentService.bindVerified(
        payload.timerAssignmentId,
        payload.expectedAssignmentRevision,
        payload.expectedIntentRevision,
        payload.backendGeneration,
        verified.binding.nativeTimerBindingId,
        verified.binding.bindingRevision,
        reconciledAt);
    if (!fulfilled.ok())
    {
        return result(
            Status::fulfillmentFailed,
            "native_timer_create_assignment_fulfillment_failed",
            verified,
            fulfilled);
    }

    const auto completed =
        completionService.complete(expectation, reconciledAt);
    if (completed.status ==
        vdrsuite::timers::NativeTimerCreateOperationCompletionStatus::completed)
    {
        return result(
            Status::completed,
            "native_timer_create_readback_reconciliation_completed",
            verified,
            fulfilled,
            completed);
    }
    if (completed.status ==
        vdrsuite::timers::NativeTimerCreateOperationCompletionStatus::
            alreadyCompleted)
    {
        return result(
            Status::alreadyCompleted,
            "native_timer_create_readback_reconciliation_already_completed",
            verified,
            fulfilled,
            completed);
    }

    return result(
        Status::completionFailed,
        "native_timer_create_operation_completion_failed",
        verified,
        fulfilled,
        completed);
}

} // namespace vdrsuite::daemon

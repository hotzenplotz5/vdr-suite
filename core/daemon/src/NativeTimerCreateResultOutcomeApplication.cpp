#include "NativeTimerCreateResultOutcomeApplication.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentNativeTimerCreate.h"
#include "MutationOperationRepository.h"

#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::daemon
{
namespace
{
using vdrsuite::agent::BackendAgentNativeTimerCreateEvidence;
using vdrsuite::agent::BackendAgentNativeTimerCreateOutcomeCategory;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::timers::NativeTimerCreateDispatchOutcomeStatus;
using vdrsuite::timers::NativeTimerCreateExecutorOutcome;
using vdrsuite::timers::NativeTimerCreateExecutorOutcomeCategory;
using Status = NativeTimerCreateResultOutcomeApplicationStatus;

NativeTimerCreateResultOutcomeApplicationResult result(
    Status status,
    const std::string& reasonCode,
    const vdrsuite::timers::NativeTimerCreateDispatchOutcomeResult& dispatch = {})
{
    NativeTimerCreateResultOutcomeApplicationResult value;
    value.status = status;
    value.reasonCode = reasonCode;
    value.dispatch = dispatch;
    return value;
}

bool parseRevision(const std::string& text, std::uint64_t& value)
{
    if (text.empty()) return false;
    std::uint64_t parsed = 0;
    for (const char character : text)
    {
        if (character < '0' || character > '9') return false;
        const std::uint64_t digit =
            static_cast<std::uint64_t>(character - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            return false;
        parsed = parsed * 10 + digit;
    }
    if (parsed == 0) return false;
    value = parsed;
    return true;
}

bool revisionAdvancedBy(
    const std::string& reservedRevision,
    const std::string& currentRevision,
    std::uint64_t expectedDelta)
{
    std::uint64_t reserved = 0;
    std::uint64_t current = 0;
    return parseRevision(reservedRevision, reserved) &&
        parseRevision(currentRevision, current) &&
        reserved <= std::numeric_limits<std::uint64_t>::max() - expectedDelta &&
        current == reserved + expectedDelta;
}

NativeTimerCreateExecutorOutcomeCategory category(
    BackendAgentNativeTimerCreateOutcomeCategory value)
{
    switch (value)
    {
        case BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect:
            return NativeTimerCreateExecutorOutcomeCategory::rejectedWithoutEffect;
        case BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified:
            return NativeTimerCreateExecutorOutcomeCategory::acceptedUnverified;
        case BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown:
            return NativeTimerCreateExecutorOutcomeCategory::outcomeUnknown;
    }
    return NativeTimerCreateExecutorOutcomeCategory::outcomeUnknown;
}

MutationOperationState targetState(
    BackendAgentNativeTimerCreateOutcomeCategory value)
{
    switch (value)
    {
        case BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect:
            return MutationOperationState::failedVerified;
        case BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified:
            return MutationOperationState::executedUnverified;
        case BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown:
            return MutationOperationState::outcomeUnknown;
    }
    return MutationOperationState::outcomeUnknown;
}
} // namespace

NativeTimerCreateResultOutcomeApplicationResult
applyDurableNativeTimerCreateResult(
    BackendAgentCommandRepository& commandRepository,
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    vdrsuite::timers::NativeTimerCreateDispatchService& dispatchService,
    const std::string& commandId)
{
    const auto assignment = commandRepository.findAssignment(commandId);
    if (!assignment.has_value())
        return result(Status::assignmentNotFound, "native_timer_create_assignment_not_found");
    if (assignment->commandType !=
        vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType)
        return result(Status::commandTypeConflict, "native_timer_create_command_type_conflict");

    const auto durableResult = commandRepository.resultForCommand(commandId);
    if (!durableResult.has_value())
        return result(Status::resultNotFound, "native_timer_create_result_not_found");

    BackendAgentNativeTimerCreateEvidence evidence;
    std::string evidenceReason;
    if (durableResult->resultEvidence.empty() ||
        !vdrsuite::agent::backendAgentNativeTimerCreateParseResultEvidence(
            durableResult->resultEvidence, *assignment, evidence, evidenceReason) ||
        evidence.completedAt != durableResult->completedAt)
    {
        return result(
            Status::evidenceInvalid,
            evidenceReason.empty()
                ? "native_timer_create_result_evidence_invalid"
                : evidenceReason);
    }

    const auto foundOperation = operationRepository.findById(evidence.operationId);
    if (foundOperation.status == MutationOperationRepositoryStatus::notFound)
        return result(Status::operationNotFound, "native_timer_create_operation_not_found");
    if (!foundOperation.ok())
        return result(
            Status::operationRepositoryError,
            "native_timer_create_operation_lookup_failed");

    const auto& operation = foundOperation.operation;
    const MutationOperationState expectedTarget = targetState(evidence.outcome);
    if (operation.state == MutationOperationState::dispatching)
    {
        // Reservation captured the accepted revision. claimAfterReservation()
        // owns the single transition to the dispatching revision.
        if (!revisionAdvancedBy(
                evidence.operationRevision, operation.operationRevision, 1))
            return result(
                Status::operationRevisionConflict,
                "native_timer_create_dispatch_revision_conflict");
    }
    else if (operation.state == expectedTarget)
    {
        // Exact replay after applyOutcome() is one further durable transition.
        if (!revisionAdvancedBy(
                evidence.operationRevision, operation.operationRevision, 2))
            return result(
                Status::operationRevisionConflict,
                "native_timer_create_applied_revision_conflict");
    }
    else
    {
        return result(
            Status::operationStateConflict,
            "native_timer_create_operation_state_conflict");
    }

    NativeTimerCreateExecutorOutcome outcome;
    outcome.operationId = evidence.operationId;

    // operationRevision is a Control-Plane transition fence, not native
    // executor evidence. Use the current repository-owned revision; never
    // reinterpret the reservation revision as the dispatch revision.
    outcome.operationRevision = operation.operationRevision;
    outcome.reservation.commandId = evidence.commandId;
    outcome.reservation.requestFingerprint = evidence.requestFingerprint;
    outcome.category = category(evidence.outcome);
    outcome.dispatchStartedAt = evidence.dispatchStartedAt;
    outcome.completedAt = evidence.completedAt;
    outcome.evidenceReference = evidence.evidenceReference;

    const auto applied = dispatchService.applyOutcome(outcome);
    if (applied.status == NativeTimerCreateDispatchOutcomeStatus::applied)
        return result(Status::applied, "native_timer_create_outcome_applied", applied);
    if (applied.status == NativeTimerCreateDispatchOutcomeStatus::alreadyApplied)
        return result(
            Status::alreadyApplied,
            "native_timer_create_outcome_already_applied",
            applied);
    return result(
        Status::dispatchRejected,
        "native_timer_create_outcome_application_rejected",
        applied);
}

} // namespace vdrsuite::daemon

#include "EpgTimerIntentAtomicAdmissionService.h"
#include "EpgTimerIntentAdmissionRepository.h"

#include "Database.h"
#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentApplicationService.h"
#include "TimerIntentRepository.h"


#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::public_epg_timer
{
namespace
{
using namespace vdrsuite::timers;
using namespace vdrsuite::operations;

AtomicAdmissionResult outcome(
    AtomicAdmissionStatus status,
    const MutationOperation& operation = {})
{
    AtomicAdmissionResult result;
    result.status = status;
    result.operation = operation;
    return result;
}

bool candidateMatches(
    const AtomicAdmissionRequest& request)
{
    const auto& c = request.candidate;
    const auto& observed = request.observation;
    const auto& n = c.desiredNativeTimerSpecification;
    return c.backendId == observed.backendId
        && c.currentBackendGeneration == observed.backendGeneration
        && c.channel.backendChannelId == observed.channelId
        && c.channel.sourceId == observed.sourceId
        && c.channel.sourceChannelId == observed.channelId
        && c.channel.sourceType == request.defaults.channelSourceType
        && c.desiredNativeTimerSpecificationPresent
        && n.channelId == observed.channelId
        && n.title == request.authoritativeTitle
        && !n.title.empty()
        && n.title.size() <= 512U
        && nativeTimerSpecificationValid(n)
        && c.conflict == TimerAssignmentPlanningConflictState::confirmedClear;
}

AtomicAdmissionStatus nativeStatus(NativeTimerCreateAdmissionStatus status)
{
    switch (status)
    {
    case NativeTimerCreateAdmissionStatus::prepared:
        return AtomicAdmissionStatus::prepared;
    case NativeTimerCreateAdmissionStatus::replayed:
        return AtomicAdmissionStatus::replayed;
    case NativeTimerCreateAdmissionStatus::idempotencyConflict:
        return AtomicAdmissionStatus::idempotencyConflict;
    case NativeTimerCreateAdmissionStatus::generationConflict:
        return AtomicAdmissionStatus::generationConflict;
    case NativeTimerCreateAdmissionStatus::assignmentNotFound:
    case NativeTimerCreateAdmissionStatus::assignmentRevisionConflict:
    case NativeTimerCreateAdmissionStatus::assignmentStateConflict:
    case NativeTimerCreateAdmissionStatus::backendConflict:
    case NativeTimerCreateAdmissionStatus::specificationUnavailable:
    case NativeTimerCreateAdmissionStatus::intentRevisionConflict:
    case NativeTimerCreateAdmissionStatus::operationConflict:
        return AtomicAdmissionStatus::stateConflict;
    case NativeTimerCreateAdmissionStatus::invalid:
        return AtomicAdmissionStatus::invalid;
    case NativeTimerCreateAdmissionStatus::identityGenerationFailed:
    case NativeTimerCreateAdmissionStatus::repositoryError:
        return AtomicAdmissionStatus::storageError;
    }
    return AtomicAdmissionStatus::storageError;
}
} // namespace

EpgTimerIntentAtomicAdmissionService::EpgTimerIntentAtomicAdmissionService(
    Database& database, TimerIntentRepository& intents,
    TimerAssignmentRepository& assignments,
    TimerIntentApplicationService& application,
    NativeTimerCreateAdmissionService& nativeCreate)
    : database_(database), intents_(intents), assignments_(assignments),
      application_(application), nativeCreate_(nativeCreate)
{
}

bool EpgTimerIntentAtomicAdmissionService::ensureSchema()
{
    EpgTimerIntentAdmissionRepository repository(database_);
    return repository.ensureSchema();
}

AtomicAdmissionResult EpgTimerIntentAtomicAdmissionService::admit(
    const AtomicAdmissionRequest& request)
{
    const Draft draft = makeProgrammeEventDraft(
        request.selection, request.observation, request.authenticatedActorId,
        request.defaults, true, true, request.requestedAt);
    if (draft.guard == Decision::revisionConflict)
        return outcome(AtomicAdmissionStatus::revisionConflict);
    if (draft.guard != Decision::ready || !draft.valid ||
        request.authenticatedActorId.empty() ||
        request.deadline < 0 ||
        (request.deadline != 0 && request.deadline < request.requestedAt))
        return outcome(AtomicAdmissionStatus::invalid);
    if (!candidateMatches(request))
        return outcome(AtomicAdmissionStatus::backendUnavailable);

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema() || !database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return outcome(AtomicAdmissionStatus::storageError);
    const auto fail = [this](AtomicAdmissionStatus status) {
        database_.execute("ROLLBACK;");
        return outcome(status);
    };
    EpgTimerIntentAdmissionRepository admissionRepository(database_);
    const auto existing = admissionRepository.lookup(
        request.authenticatedActorId, request.selection);
    if (existing.status == AdmissionReplayStatus::storageError)
        return fail(AtomicAdmissionStatus::storageError);
    if (existing.status == AdmissionReplayStatus::conflict)
        return fail(AtomicAdmissionStatus::idempotencyConflict);
    if (existing.status == AdmissionReplayStatus::same)
    {
        MutationOperationRepository repository(database_);
        const auto op = repository.findById(existing.operationId);
        if (!op.ok() || op.operation.actorId != request.authenticatedActorId ||
            op.operation.backendId != request.selection.backendId ||
            op.operation.actionFamily != "timer.create")
            return fail(AtomicAdmissionStatus::storageError);
        if (!database_.execute("COMMIT;"))
            return fail(AtomicAdmissionStatus::storageError);
        return outcome(AtomicAdmissionStatus::replayed, op.operation);
    }

    // Default one-shot policy never silently creates two native timers for
    // an already active semantically equivalent scheduling intent.
    const auto equivalents = intents_.findEquivalent(draft.specification);
    if (!equivalents.ok())
        return fail(AtomicAdmissionStatus::storageError);
    for (const auto& intent : equivalents.intents)
        if (!timerIntentTerminal(intent.state))
            return fail(AtomicAdmissionStatus::duplicateIntent);

    const std::string intentId = "epg-intent:" + generateMutationOperationId();
    const std::string assignmentId =
        "epg-assignment:" + generateMutationOperationId();
    if (intentId.size() > 160 || assignmentId.size() > 160 ||
        !mutationOperationIdCanonical(intentId.substr(11)) ||
        !mutationOperationIdCanonical(assignmentId.substr(15)) ||
        request.requestedAt > std::numeric_limits<std::int64_t>::max() - 4)
        return fail(AtomicAdmissionStatus::storageError);

    TimerIntentApplicationRequest application;
    application.intent.timerIntentId = intentId;
    application.intent.state = TimerIntentState::draft;
    application.intent.spec = draft.specification;
    application.intent.createdByActorId = request.authenticatedActorId;
    application.intent.createdAt = request.requestedAt;
    application.intent.updatedAt = request.requestedAt;
    application.intent.expiresAt = request.observation.endTime;
    application.timerAssignmentId = assignmentId;
    application.candidates = {request.candidate};
    application.activatedAt = request.requestedAt + 1;
    application.scheduledAt = request.requestedAt + 2;
    application.provisioningAt = request.requestedAt + 3;
    const auto scheduled = application_.submitAndSelectPrimaryInCurrentTransaction(
        application);
    if (scheduled.status != TimerIntentApplicationStatus::assignmentSelected)
        return fail(scheduled.status == TimerIntentApplicationStatus::noEligibleBackend
            ? AtomicAdmissionStatus::backendUnavailable
            : AtomicAdmissionStatus::stateConflict);
    if (scheduled.assignment.backendId != request.selection.backendId ||
        scheduled.assignment.backendGeneration !=
            request.observation.backendGeneration ||
        scheduled.assignment.state != TimerAssignmentState::selected)
        return fail(AtomicAdmissionStatus::generationConflict);

    NativeTimerCreateAdmissionRequest native;
    native.actorId = request.authenticatedActorId;
    native.idempotencyKey = request.selection.idempotencyKey;
    native.timerAssignmentId = assignmentId;
    native.expectedAssignmentRevision =
        scheduled.assignment.assignmentRevision;
    native.expectedBackendId = request.selection.backendId;
    native.requestedAt = request.requestedAt + 3;
    native.deadline = request.deadline;
    const auto prepared = nativeCreate_.admitInCurrentTransaction(native);
    if (prepared.status != NativeTimerCreateAdmissionStatus::prepared)
        return fail(nativeStatus(prepared.status));

    if (!admissionRepository.record(
            request.authenticatedActorId, request.selection, intentId,
            assignmentId, prepared.operation.operationId))
        return fail(AtomicAdmissionStatus::storageError);

    if (!database_.execute("COMMIT;"))
        return fail(AtomicAdmissionStatus::storageError);
    return outcome(AtomicAdmissionStatus::prepared, prepared.operation);
}

} // namespace vdrsuite::public_epg_timer

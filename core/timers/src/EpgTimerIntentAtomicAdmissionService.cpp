#include "EpgTimerIntentAtomicAdmissionService.h"

#include "Database.h"
#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentApplicationService.h"
#include "TimerIntentRepository.h"

#include <sqlite3.h>

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

bool bind(sqlite3_stmt* statement, int index, const std::string& value)
{
    return value.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max())
        && sqlite3_bind_text(statement, index, value.data(),
            static_cast<int>(value.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string column(sqlite3_stmt* statement, int index)
{
    const unsigned char* value = sqlite3_column_text(statement, index);
    const int size = sqlite3_column_bytes(statement, index);
    return value && size >= 0
        ? std::string(reinterpret_cast<const char*>(value), size)
        : std::string();
}

enum class Existing { none, same, conflict, error };

Existing replayLookup(
    Database& database, const AtomicAdmissionRequest& request,
    std::string& operationId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT channel_id,event_id,event_etag,operation_id "
        "FROM epg_timer_intent_admissions "
        "WHERE actor_id=? AND backend_id=? AND idempotency_key=?;";
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK)
        return Existing::error;
    const bool bound = bind(statement, 1, request.authenticatedActorId)
        && bind(statement, 2, request.selection.backendId)
        && bind(statement, 3, request.selection.idempotencyKey);
    const int rc = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    Existing result = Existing::error;
    if (rc == SQLITE_DONE) result = Existing::none;
    if (rc == SQLITE_ROW)
    {
        result = column(statement, 0) == request.selection.channelId
            && column(statement, 1) == request.selection.eventId
            && column(statement, 2) == request.selection.ifMatch
            ? Existing::same : Existing::conflict;
        if (result == Existing::same) operationId = column(statement, 3);
    }
    sqlite3_finalize(statement);
    return result;
}

bool recordAdmission(Database& database, const AtomicAdmissionRequest& request,
    const std::string& intentId, const std::string& assignmentId,
    const std::string& operationId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_timer_intent_admissions "
        "(actor_id,backend_id,idempotency_key,channel_id,event_id,"
        "event_etag,intent_id,assignment_id,operation_id) "
        "VALUES (?,?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK)
        return false;
    const bool bound = bind(statement, 1, request.authenticatedActorId)
        && bind(statement, 2, request.selection.backendId)
        && bind(statement, 3, request.selection.idempotencyKey)
        && bind(statement, 4, request.selection.channelId)
        && bind(statement, 5, request.selection.eventId)
        && bind(statement, 6, request.selection.ifMatch)
        && bind(statement, 7, intentId)
        && bind(statement, 8, assignmentId)
        && bind(statement, 9, operationId);
    const int rc = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    sqlite3_finalize(statement);
    return rc == SQLITE_DONE;
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
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS epg_timer_intent_admissions ("
        "actor_id TEXT NOT NULL, backend_id TEXT NOT NULL,"
        "idempotency_key TEXT NOT NULL, channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL, event_etag TEXT NOT NULL,"
        "intent_id TEXT NOT NULL, assignment_id TEXT NOT NULL,"
        "operation_id TEXT NOT NULL UNIQUE,"
        "PRIMARY KEY(actor_id,backend_id,idempotency_key),"
        "FOREIGN KEY(intent_id) REFERENCES timer_intents(timer_intent_id),"
        "FOREIGN KEY(assignment_id) REFERENCES timer_assignments(timer_assignment_id),"
        "FOREIGN KEY(operation_id) REFERENCES mutation_operations(operation_id)"
        ");");
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
    std::string priorId;
    const Existing existing = replayLookup(database_, request, priorId);
    if (existing == Existing::error)
        return fail(AtomicAdmissionStatus::storageError);
    if (existing == Existing::conflict)
        return fail(AtomicAdmissionStatus::idempotencyConflict);
    if (existing == Existing::same)
    {
        MutationOperationRepository repository(database_);
        const auto op = repository.findById(priorId);
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

    if (!recordAdmission(database_, request, intentId,
            assignmentId, prepared.operation.operationId))
        return fail(AtomicAdmissionStatus::storageError);

    if (!database_.execute("COMMIT;"))
        return fail(AtomicAdmissionStatus::storageError);
    return outcome(AtomicAdmissionStatus::prepared, prepared.operation);
}

} // namespace vdrsuite::public_epg_timer

#pragma once

#include "MutationOperation.h"
#include "NativeTimerCreateOperationPayload.h"
#include "ManualTimerIntentDraft.h"
#include "EpgNativeTimerTimeGuard.h"
#include "EpgNativeTimerConflictGuard.h"
#include "EpgNativeTimerExecutionAuthority.h"
#include "TimerAssignmentPlanner.h"

#include <cstdint>
#include <functional>
#include <string>

class Database;

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerCreateOperationPreparationService;
class TimerAssignmentFulfillmentService;
class TimerAssignmentRepository;
class TimerIntentApplicationService;
class TimerIntentRepository;

enum class NativeTimerCreateAdmissionStatus
{
    prepared,
    replayed,
    assignmentNotFound,
    assignmentRevisionConflict,
    assignmentStateConflict,
    backendConflict,
    specificationUnavailable,
    intentRevisionConflict,
    generationConflict,
    idempotencyConflict,
    operationConflict,
    identityGenerationFailed,
    repositoryError,
    invalid,
};


 // Manual and recurring draft admission reuse the EXISTING native CREATE
 // operation authority. Input projection/Planner evidence is server-owned;
 // the API may supply desired fields but not authoritative Agent facts.

struct ManualNativeTimerCreateReplayRequest
{
    std::string actorId, backendId, idempotencyKey;
    std::string channelId, title, directory;
    // Explicit native recurrence mask; the original one-shot contract
    // continues to use ------- by default.
    std::string weekdays = "-------";
    std::int64_t startAt = 0, stopAt = 0;
    std::int32_t priority = 50, lifetime = 99;
    bool enabled = true, vps = false;
};

struct ManualNativeTimerCreateAdmissionRequest
{
    std::string actorId;
    std::string backendId;
    std::string idempotencyKey;
    ManualTimerIntentDraft draft;
    TimerAssignmentPlanningBackendCandidate candidate;
    vdrsuite::public_epg_timer::ServerOwnedNativeClockEvidence nativeClock;
    vdrsuite::public_epg_timer::ProspectiveNativeTimerConflictProof conflictProof;
    BackendAgentRecord authoritativeAgent;
    vdrsuite::agent::BackendAgentLocalProviderSelection nativeCreateProvider;
    std::int64_t requestedAt = 0;
    std::int64_t deadline = 0;
    // Re-evaluate current backend/Agent/provider and channel mapping under
    // the already-established atomic transaction; no implicit retry.
    std::function<bool()> revalidateSourceAndExecutionAuthority;
};

struct NativeTimerCreateAdmissionRequest
{
    std::string actorId;
    std::string idempotencyKey;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedBackendId;
    std::int64_t requestedAt = 0;
    std::int64_t deadline = 0;
};

struct NativeTimerCreateAdmissionResult
{
    NativeTimerCreateAdmissionStatus status =
        NativeTimerCreateAdmissionStatus::repositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerCreateOperationPayload payload;

    bool ok() const
    {
        return status == NativeTimerCreateAdmissionStatus::prepared
            || status == NativeTimerCreateAdmissionStatus::replayed;
    }
};

class NativeTimerCreateAdmissionService
{
public:
    NativeTimerCreateAdmissionService(
        Database& database,
        TimerAssignmentRepository& assignmentRepository,
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        TimerAssignmentFulfillmentService& fulfillmentService,
        NativeTimerCreateOperationPreparationService& preparationService);

    NativeTimerCreateAdmissionResult admit(
        const NativeTimerCreateAdmissionRequest& request);

    // One transaction owns manual Intent, selected Assignment and the same
    // NativeTimerCreateAdmissionService Operation/payload. No second writer
    // and no second idempotency table: deterministic IDs reserve a key scope.
    NativeTimerCreateAdmissionResult admitManualWindow(
        const ManualNativeTimerCreateAdmissionRequest& request,
        TimerIntentRepository& intents,
        TimerIntentApplicationService& application);

    // Read-only recovery; no Agent/Clock/preview needed for a committed
    // Operation. Returns assignmentNotFound for a previously unused key.
    NativeTimerCreateAdmissionResult replayManualWindow(
        const ManualNativeTimerCreateReplayRequest& request,
        TimerIntentRepository& intents);

    // Higher-level EPG admission owns BEGIN/COMMIT/ROLLBACK and the
    // database transaction lease. Never commits a partially-created
    // TimerIntent/Assignment; the caller must roll back on ANY failure.
    // No standalone mutation without an active transaction.
    NativeTimerCreateAdmissionResult admitInCurrentTransaction(
        const NativeTimerCreateAdmissionRequest& request);

private:
    NativeTimerCreateAdmissionResult admitImpl(
        const NativeTimerCreateAdmissionRequest& request,
        bool currentTransaction);
    NativeTimerCreateAdmissionResult replayIfPresent(
        const NativeTimerCreateAdmissionRequest& request,
        const std::string& requestFingerprint,
        bool& present);

    Database& database_;
    TimerAssignmentRepository& assignmentRepository_;
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    TimerAssignmentFulfillmentService& fulfillmentService_;
    NativeTimerCreateOperationPreparationService& preparationService_;
};

} // namespace vdrsuite::timers

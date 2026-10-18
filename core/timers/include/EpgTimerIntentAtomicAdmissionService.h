#pragma once

#include "EpgTimerIntentDraft.h"
#include "NativeTimerCreateAdmissionService.h"
#include "TimerAssignmentPlanner.h"

#include <cstdint>
#include <string>

class Database;

namespace vdrsuite::timers
{
class TimerIntentApplicationService;
class TimerIntentRepository;
class TimerAssignmentRepository;
}

namespace vdrsuite::public_epg_timer
{

enum class AtomicAdmissionStatus
{
    prepared,
    replayed,
    invalid,
    revisionConflict,
    generationConflict,
    duplicateIntent,
    stateConflict,
    idempotencyConflict,
    backendUnavailable,
    storageError
};

struct AtomicAdmissionRequest
{
    Selection selection;
    AuthoritativeObservation observation;
    ServerOwnedRecordingDefaults defaults;
    std::string authenticatedActorId;
    // Native timer description and planning evidence MUST come from the
    // authoritative backend/server owner, not from Device request JSON.
    std::string authoritativeTitle;
    vdrsuite::timers::TimerAssignmentPlanningBackendCandidate candidate;
    std::int64_t requestedAt = 0;
    std::int64_t deadline = 0;
};

struct AtomicAdmissionResult
{
    AtomicAdmissionStatus status = AtomicAdmissionStatus::storageError;
    vdrsuite::operations::MutationOperation operation;
};

class EpgTimerIntentAtomicAdmissionService
{
public:
    EpgTimerIntentAtomicAdmissionService(
        Database& database,
        vdrsuite::timers::TimerIntentRepository& intents,
        vdrsuite::timers::TimerAssignmentRepository& assignments,
        vdrsuite::timers::TimerIntentApplicationService& application,
        vdrsuite::timers::NativeTimerCreateAdmissionService& nativeCreate);

    bool ensureSchema();
    AtomicAdmissionResult admit(const AtomicAdmissionRequest& request);

private:
    Database& database_;
    vdrsuite::timers::TimerIntentRepository& intents_;
    vdrsuite::timers::TimerAssignmentRepository& assignments_;
    vdrsuite::timers::TimerIntentApplicationService& application_;
    vdrsuite::timers::NativeTimerCreateAdmissionService& nativeCreate_;
};

} // namespace vdrsuite::public_epg_timer

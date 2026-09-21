#pragma once

#include "TimerAssignment.h"

#include <string>
#include <vector>

class Database;

namespace vdrsuite::timers
{

enum class TimerAssignmentRepositoryStatus
{
    ok,
    invalid,
    notFound,
    conflict,
    alreadyExists,
    ownershipConflict,
    intentNotFound,
    intentRevisionConflict,
    storageError,
};

struct TimerAssignmentRepositoryResult
{
    TimerAssignmentRepositoryStatus status =
        TimerAssignmentRepositoryStatus::storageError;
    TimerAssignment assignment;

    bool ok() const
    {
        return status == TimerAssignmentRepositoryStatus::ok;
    }
};

struct TimerAssignmentRepositoryListResult
{
    TimerAssignmentRepositoryStatus status =
        TimerAssignmentRepositoryStatus::storageError;
    std::vector<TimerAssignment> assignments;

    bool ok() const
    {
        return status == TimerAssignmentRepositoryStatus::ok;
    }
};

struct TimerAssignmentRepositorySetRevisionResult
{
    TimerAssignmentRepositoryStatus status =
        TimerAssignmentRepositoryStatus::storageError;
    std::string assignmentSetRevision;

    bool ok() const
    {
        return status == TimerAssignmentRepositoryStatus::ok;
    }
};

enum class TimerAssignmentReassignmentNativeOutcome
{
    beforeDispatch,
    verifiedAbsent,
};

struct TimerAssignmentReassignmentEvidence
{
    std::string oldTimerAssignmentId;
    std::string oldAssignmentRevision;
    std::uint64_t oldAssignmentEpoch = 0;
    std::string oldBackendId;
    std::uint64_t oldBackendGeneration = 0;
    TimerAssignmentReassignmentNativeOutcome oldNativeOutcome =
        TimerAssignmentReassignmentNativeOutcome::beforeDispatch;
    std::string oldOperationId;
    std::string oldOperationRevision;
    std::string oldNativeTimerBindingId;
    std::string oldBindingRevision;
    std::string reason;
    std::string replacementTimerAssignmentId;
    std::string newBackendId;
    std::uint64_t newBackendGeneration = 0;
    std::uint64_t newAssignmentEpoch = 0;
    std::int64_t createdAt = 0;
};

struct TimerAssignmentControlledReplacementResult
{
    TimerAssignmentRepositoryStatus status =
        TimerAssignmentRepositoryStatus::storageError;
    TimerAssignment oldAssignment;
    TimerAssignment replacementAssignment;
    TimerAssignmentReassignmentEvidence evidence;

    bool ok() const
    {
        return status == TimerAssignmentRepositoryStatus::ok;
    }
};

class TimerAssignmentRepository
{
public:
    explicit TimerAssignmentRepository(Database& database);

    bool ensureSchema();

    // Creation owns both the initial assignment revision and the next
    // per-intent assignment epoch. Callers must leave assignmentRevision empty
    // and assignmentEpoch at zero.
    TimerAssignmentRepositoryResult create(
        const TimerAssignment& assignment);

    // Snapshot-fenced creation reuses create() for the durable assignment write.
    // The opaque set revision represents the complete TimerAssignment set for
    // one TimerIntent. A stale set revision returns `conflict`; the repository
    // still owns assignmentRevision, assignmentEpoch and primary ownership.
    TimerAssignmentRepositoryResult createAgainstAssignmentSetRevision(
        const TimerAssignment& assignment,
        const std::string& expectedAssignmentSetRevision);

    // Atomically closes one exact active exclusive owner, creates one new
    // replacement assignment and persists the handover evidence. The set,
    // assignment revision/epoch and backend generation fences are rechecked at
    // the INSERT boundary. Callers must validate the native-outcome evidence.
    TimerAssignmentControlledReplacementResult createControlledReplacement(
        const TimerAssignment& replacement,
        const std::string& expectedAssignmentSetRevision,
        const TimerAssignmentReassignmentEvidence& evidence);

    TimerAssignmentControlledReplacementResult findControlledReplacement(
        const std::string& replacementTimerAssignmentId);

    TimerAssignmentRepositoryResult findById(
        const std::string& timerAssignmentId);

    TimerAssignmentRepositoryListResult listForIntent(
        const std::string& timerIntentId);

    // Returns an opaque per-intent set revision. Callers that plan from a
    // subsequently loaded assignment list can safely persist only against this
    // exact token: any intervening assignment create/update/delete invalidates
    // the token.
    TimerAssignmentRepositorySetRevisionResult
    assignmentSetRevisionForIntent(
        const std::string& timerIntentId);

    TimerAssignmentRepositoryResult findActivePrimaryForIntent(
        const std::string& timerIntentId);

    // Update is exact optimistic concurrency. The stable assignment identity,
    // owning TimerIntent, repository-issued epoch, role and creation time are
    // immutable. A previously selected backend cannot silently change.
    TimerAssignmentRepositoryResult update(
        const TimerAssignment& next,
        const std::string& expectedRevision);

private:
    Database& database_;
};

}

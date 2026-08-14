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

    TimerAssignmentRepositoryResult findById(
        const std::string& timerAssignmentId);

    TimerAssignmentRepositoryListResult listForIntent(
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

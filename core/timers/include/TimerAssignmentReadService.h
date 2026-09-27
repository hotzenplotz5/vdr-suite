#pragma once

#include "TimerAssignment.h"

#include <cstddef>
#include <string>
#include <vector>

namespace vdrsuite::timers
{

class TimerAssignmentRepository;

enum class TimerAssignmentReadStatus
{
    ok,
    invalid,
    notFound,
    storageError,
};

struct TimerAssignmentReadResult
{
    TimerAssignmentReadStatus status =
        TimerAssignmentReadStatus::storageError;
    TimerAssignment assignment;

    bool ok() const
    {
        return status == TimerAssignmentReadStatus::ok;
    }
};

enum class TimerAssignmentCollectionReadStatus
{
    ok,
    invalid,
    storageError,
};

struct TimerAssignmentCollectionReadResult
{
    TimerAssignmentCollectionReadStatus status =
        TimerAssignmentCollectionReadStatus::storageError;
    std::vector<TimerAssignment> assignments;
    bool hasMore = false;

    bool ok() const
    {
        return status == TimerAssignmentCollectionReadStatus::ok;
    }
};

// Read-only facade for public/application consumers.
//
// TimerAssignmentRepository remains the single Phase-64 persistence and revision
// authority. Callers must authorize the requested backend before exposing a
// successful result. A real assignment that belongs to another backend is
// deliberately reported as notFound so an authorized backend scope cannot be
// used as a cross-backend assignment existence oracle.
class TimerAssignmentReadService
{
public:
    explicit TimerAssignmentReadService(
        TimerAssignmentRepository& repository);

    TimerAssignmentReadResult findForBackend(
        const std::string& timerAssignmentId,
        const std::string& backendId) const;

    // Public collection facade: single backend, immutable identity keyset,
    // bounded look-ahead. It deliberately does not aggregate backend sources.
    TimerAssignmentCollectionReadResult listForBackend(
        const std::string& backendId,
        const std::string& afterTimerAssignmentId,
        std::size_t limit) const;

private:
    TimerAssignmentRepository& repository_;
};

}

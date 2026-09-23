#pragma once

#include "TimerAssignment.h"

#include <string>

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

private:
    TimerAssignmentRepository& repository_;
};

}

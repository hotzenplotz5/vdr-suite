#include "TimerAssignmentReadService.h"

#include "TimerAssignmentRepository.h"

namespace vdrsuite::timers
{

namespace
{

TimerAssignmentReadResult result(
    TimerAssignmentReadStatus status,
    const TimerAssignment& assignment = {})
{
    TimerAssignmentReadResult value;
    value.status = status;
    value.assignment = assignment;
    return value;
}

}

TimerAssignmentReadService::TimerAssignmentReadService(
    TimerAssignmentRepository& repository)
    : repository_(repository)
{
}

TimerAssignmentReadResult TimerAssignmentReadService::findForBackend(
    const std::string& timerAssignmentId,
    const std::string& backendId) const
{
    if (timerAssignmentId.empty() || backendId.empty())
    {
        return result(TimerAssignmentReadStatus::invalid);
    }

    const TimerAssignmentRepositoryResult found =
        repository_.findById(timerAssignmentId);

    if (found.status == TimerAssignmentRepositoryStatus::invalid)
    {
        return result(TimerAssignmentReadStatus::invalid);
    }

    if (found.status == TimerAssignmentRepositoryStatus::notFound)
    {
        return result(TimerAssignmentReadStatus::notFound);
    }

    if (found.status != TimerAssignmentRepositoryStatus::ok)
    {
        return result(TimerAssignmentReadStatus::storageError);
    }

    if (found.assignment.backendId != backendId)
    {
        return result(TimerAssignmentReadStatus::notFound);
    }

    return result(
        TimerAssignmentReadStatus::ok,
        found.assignment);
}

}

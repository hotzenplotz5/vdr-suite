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

TimerAssignmentCollectionReadResult
TimerAssignmentReadService::listForBackend(
    const std::string& backendId,
    const std::string& afterTimerAssignmentId,
    std::size_t limit) const
{
    TimerAssignmentCollectionReadResult result;

    constexpr std::size_t kMaximumPublicLimit = 100U;
    if (backendId.empty() ||
        limit == 0U ||
        limit > kMaximumPublicLimit)
    {
        result.status = TimerAssignmentCollectionReadStatus::invalid;
        return result;
    }

    const TimerAssignmentRepositoryListResult listed =
        repository_.listForBackendAfter(
            backendId,
            afterTimerAssignmentId,
            limit + 1U);

    if (listed.status == TimerAssignmentRepositoryStatus::invalid)
    {
        result.status = TimerAssignmentCollectionReadStatus::invalid;
        return result;
    }
    if (listed.status != TimerAssignmentRepositoryStatus::ok)
    {
        result.status =
            TimerAssignmentCollectionReadStatus::storageError;
        return result;
    }

    std::string previous = afterTimerAssignmentId;
    for (const TimerAssignment& assignment : listed.assignments)
    {
        if (assignment.backendId != backendId ||
            assignment.timerAssignmentId.empty() ||
            (!previous.empty() &&
             assignment.timerAssignmentId <= previous))
        {
            result.assignments.clear();
            result.status =
                TimerAssignmentCollectionReadStatus::storageError;
            return result;
        }
        previous = assignment.timerAssignmentId;
    }

    result.assignments = listed.assignments;
    if (result.assignments.size() > limit)
    {
        result.assignments.resize(limit);
        result.hasMore = true;
    }
    result.status = TimerAssignmentCollectionReadStatus::ok;
    return result;
}

}

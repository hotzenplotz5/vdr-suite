#pragma once

#include "TimerAssignmentFulfillmentService.h"
#include "TimerAssignmentSchedulingService.h"
#include "TimerIntent.h"

#include <cstdint>
#include <vector>

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;
class TimerAssignmentRepository;
class TimerIntentRepository;

enum class TimerIntentApplicationStatus
{
    provisioningStarted,
    assignmentSelected,
    alreadyProvisioning,
    alreadyBound,
    noEligibleBackend,
    intentConflict,
    assignmentConflict,
    schedulingConflict,
    provisioningConflict,
    repositoryError,
    invalid,
};

struct TimerIntentApplicationRequest
{
    TimerIntent intent;
    std::string timerAssignmentId;
    std::vector<TimerAssignmentPlanningBackendCandidate> candidates;
    std::int64_t activatedAt = 0;
    std::int64_t scheduledAt = 0;
    std::int64_t provisioningAt = 0;
};

struct TimerIntentApplicationResult
{
    TimerIntentApplicationStatus status =
        TimerIntentApplicationStatus::repositoryError;
    TimerIntent intent;
    TimerAssignment assignment;

    bool ok() const
    {
        return status == TimerIntentApplicationStatus::assignmentSelected
            || status == TimerIntentApplicationStatus::provisioningStarted
            || status == TimerIntentApplicationStatus::alreadyProvisioning
            || status == TimerIntentApplicationStatus::alreadyBound;
    }
};

class TimerIntentApplicationService
{
public:
    TimerIntentApplicationService(
        TimerIntentRepository& intentRepository,
        TimerAssignmentRepository& assignmentRepository,
        NativeTimerBindingRepository& bindingRepository);

    // Persists/reuses the same Intent and primary Assignment without
    // beginning provisioning. The existing atomic NativeTimerCreateAdmission
    // service exclusively owns that transition for public-v1 CREATE.
    TimerIntentApplicationResult submitAndSelectPrimary(
        const TimerIntentApplicationRequest& request);

    // Does not commit: the EPG admission coordinator owns the entire
    // Intent -> Assignment -> Operation transaction and rollback.
    TimerIntentApplicationResult submitAndSelectPrimaryInCurrentTransaction(
        const TimerIntentApplicationRequest& request);

    // Retained for existing owners with deliberate immediate provisioning.
    TimerIntentApplicationResult submitAndProvisionPrimary(
        const TimerIntentApplicationRequest& request);

private:
    TimerIntentApplicationResult submitPrimary(
        const TimerIntentApplicationRequest& request,
        bool provisionImmediately,
        bool currentTransaction);
    TimerIntentRepository& intentRepository_;
    TimerAssignmentRepository& assignmentRepository_;
    TimerAssignmentSchedulingService scheduling_;
    TimerAssignmentFulfillmentService fulfillment_;
};

}

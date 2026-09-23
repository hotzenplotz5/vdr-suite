#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();

    runtime.resetTimerAssignmentLookup();
    assert(!runtime.timerAssignmentLookupConfigured());

    const auto unavailable =
        runtime.lookupTimerAssignment(
            "assignment:one",
            "backend:one");
    assert(
        unavailable.status ==
        PublicTimerAssignmentLookupStatus::unavailable);

    runtime.registerTimerAssignmentLookup(
        [](
            const std::string& timerAssignmentId,
            const std::string& backendId)
        {
            PublicTimerAssignmentLookupResult result;

            if (timerAssignmentId.empty() || backendId.empty())
            {
                result.status =
                    PublicTimerAssignmentLookupStatus::invalid;
                return result;
            }

            if (timerAssignmentId != "assignment:one" ||
                backendId != "backend:one")
            {
                result.status =
                    PublicTimerAssignmentLookupStatus::notFound;
                return result;
            }

            result.status =
                PublicTimerAssignmentLookupStatus::ok;
            result.assignment.timerAssignmentId =
                timerAssignmentId;
            result.assignment.backendId =
                backendId;
            result.assignment.resourceRevision =
                "revision:7";
            return result;
        });

    assert(runtime.timerAssignmentLookupConfigured());

    const auto found =
        runtime.lookupTimerAssignment(
            "assignment:one",
            "backend:one");
    assert(found.status ==
        PublicTimerAssignmentLookupStatus::ok);
    assert(found.assignment.timerAssignmentId ==
        "assignment:one");
    assert(found.assignment.backendId ==
        "backend:one");
    assert(found.assignment.resourceRevision ==
        "revision:7");

    const auto hidden =
        runtime.lookupTimerAssignment(
            "assignment:one",
            "backend:two");
    assert(hidden.status ==
        PublicTimerAssignmentLookupStatus::notFound);

    const auto invalid =
        runtime.lookupTimerAssignment(
            "",
            "backend:one");
    assert(invalid.status ==
        PublicTimerAssignmentLookupStatus::invalid);

    ApiResponse routeStillClosed;
    assert(runtime.tryHandleGet(
        "/api/v1/timer-assignments/assignment:one?backend=backend:one",
        "actor:test",
        "phase69c-runtime-composition-request",
        "",
        routeStillClosed));
    assert(routeStillClosed.statusCode == 404);
    assert(routeStillClosed.body.find(
        "\"code\":\"not_found\"") !=
        std::string::npos);

    runtime.resetTimerAssignmentLookup();
    assert(!runtime.timerAssignmentLookupConfigured());

    return 0;
}

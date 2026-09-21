#include "NativeTimerSpecification.h"
#include "VdrManagedTimerCreateRequestBuilder.h"
#include "VdrTimerManagedCorrelation.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification()
{
    NativeTimerSpecification value;
    value.channelId = "S19.2E-1-1019-10301";
    value.title = "Managed Timer";
    value.directory = "VDR-Suite";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1015";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

VdrTimerManagedCorrelation correlation()
{
    VdrTimerManagedCorrelation value;
    value.timerAssignmentId = "assignment:1";
    value.nativeTimerBindingId = "binding:1";
    return value;
}
}

int main()
{
    const std::string foreign =
        "<epgsearch><searchtimer>42</searchtimer></epgsearch>";

    const auto built = VdrManagedTimerCreateRequestBuilder::build(
        "backend:1", specification(), correlation(), foreign);
    assert(built.ok());
    assert(built.request.timerId.empty());
    assert(built.request.backendId == "backend:1");
    assert(built.request.channelId == "S19.2E-1-1019-10301");
    assert(built.request.title == "Managed Timer");
    assert(built.request.directory == "VDR-Suite");
    assert(built.request.day == "2026-08-17");
    assert(built.request.weekdays == "-------");
    assert(built.request.start == 930);
    assert(built.request.stop == 1015);
    assert(built.request.priority == 50);
    assert(built.request.lifetime == 99);
    assert(built.request.active);
    assert(!built.request.vps);
    assert(built.request.aux.rfind(foreign, 0) == 0);

    const auto parsed = parseVdrTimerManagedCorrelation(built.request.aux);
    assert(parsed.ok());
    assert(parsed.correlation.timerAssignmentId == "assignment:1");
    assert(parsed.correlation.nativeTimerBindingId == "binding:1");

    const auto replay = VdrManagedTimerCreateRequestBuilder::build(
        "backend:1", specification(), correlation(), built.request.aux);
    assert(replay.ok());
    assert(replay.request.aux == built.request.aux);

    auto midnight = specification();
    midnight.startTime = "0000";
    const auto midnightBuilt = VdrManagedTimerCreateRequestBuilder::build(
        "backend:1", midnight, correlation());
    assert(midnightBuilt.ok());
    assert(midnightBuilt.request.start == 0);

    auto disabled = specification();
    disabled.enabled = false;
    disabled.vps = true;
    const auto disabledBuilt = VdrManagedTimerCreateRequestBuilder::build(
        "backend:1", disabled, correlation());
    assert(disabledBuilt.ok());
    assert(!disabledBuilt.request.active);
    assert(disabledBuilt.request.vps);

    assert(VdrManagedTimerCreateRequestBuilder::build(
        "", specification(), correlation()).status ==
        VdrManagedTimerCreateRequestBuildStatus::invalidBackendIdentity);

    auto invalidSpec = specification();
    invalidSpec.startTime = "2460";
    assert(VdrManagedTimerCreateRequestBuilder::build(
        "backend:1", invalidSpec, correlation()).status ==
        VdrManagedTimerCreateRequestBuildStatus::invalidSpecification);

    auto invalidCorrelation = correlation();
    invalidCorrelation.nativeTimerBindingId.clear();
    assert(VdrManagedTimerCreateRequestBuilder::build(
        "backend:1", specification(), invalidCorrelation).status ==
        VdrManagedTimerCreateRequestBuildStatus::invalidCorrelation);

    VdrTimerManagedCorrelation other = correlation();
    other.nativeTimerBindingId = "binding:other";
    const auto existing = attachVdrTimerManagedCorrelation({}, other);
    assert(existing.ok());
    assert(VdrManagedTimerCreateRequestBuilder::build(
        "backend:1", specification(), correlation(), existing.aux).status ==
        VdrManagedTimerCreateRequestBuildStatus::auxConflict);

    std::cout << "test_vdr_managed_timer_create_request_builder passed\n";
    return 0;
}

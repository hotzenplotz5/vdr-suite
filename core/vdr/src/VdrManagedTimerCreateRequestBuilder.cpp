#include "VdrManagedTimerCreateRequestBuilder.h"

#include <cstddef>

namespace
{
constexpr std::size_t kMaxIdentityLength = 160;

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

int hhmmToInt(const std::string& value)
{
    int numeric = 0;
    for (char ch : value)
        numeric = numeric * 10 + (ch - '0');
    return numeric;
}

VdrManagedTimerCreateRequestBuildResult result(
    VdrManagedTimerCreateRequestBuildStatus status)
{
    VdrManagedTimerCreateRequestBuildResult value;
    value.status = status;
    return value;
}
}

VdrManagedTimerCreateRequestBuildResult
VdrManagedTimerCreateRequestBuilder::build(
    const std::string& backendId,
    const vdrsuite::timers::NativeTimerSpecification& specification,
    const VdrTimerManagedCorrelation& correlation,
    const std::string& baseAux)
{
    if (!validIdentity(backendId))
    {
        return result(
            VdrManagedTimerCreateRequestBuildStatus::invalidBackendIdentity);
    }
    if (!vdrsuite::timers::nativeTimerSpecificationValid(specification))
    {
        return result(
            VdrManagedTimerCreateRequestBuildStatus::invalidSpecification);
    }
    if (!vdrTimerManagedCorrelationValid(correlation))
    {
        return result(
            VdrManagedTimerCreateRequestBuildStatus::invalidCorrelation);
    }

    const auto attached = attachVdrTimerManagedCorrelation(baseAux, correlation);
    if (!attached.ok())
    {
        return result(VdrManagedTimerCreateRequestBuildStatus::auxConflict);
    }

    VdrManagedTimerCreateRequestBuildResult value;
    value.status = VdrManagedTimerCreateRequestBuildStatus::ok;
    auto& request = value.request;
    request.backendId = backendId;
    request.channelId = specification.channelId;
    request.title = specification.title;
    request.directory = specification.directory;
    request.day = specification.day;
    request.weekdays = specification.weekdays;
    request.start = hhmmToInt(specification.startTime);
    request.stop = hhmmToInt(specification.endTime);
    request.priority = specification.priority;
    request.lifetime = specification.lifetime;
    request.active = specification.enabled;
    request.vps = specification.vps;
    request.aux = attached.aux;
    return value;
}

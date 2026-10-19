#include "RestfulApiNativeTimerModifyReadbackReader.h"

#include "RestfulApiNativeTimerInventoryReader.h"
#include "RestfulApiTimerMapper.h"
#include "VdrNativeTimerObservationMapper.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>

namespace vdrsuite::vdr
{
RestfulApiNativeTimerModifyReadbackReader::
RestfulApiNativeTimerModifyReadbackReader(IHttpClient& httpClient)
    : httpClient_(httpClient)
{
}

NativeTimerModifyReadResult RestfulApiNativeTimerModifyReadbackReader::read(
    const NativeTimerModifyReadRequest& request) const
{
    NativeTimerModifyReadResult result;
    if (request.backendId.empty() || request.backendId.size() > 160 ||
        request.backendNativeTimerId.empty() ||
        request.backendNativeTimerId.size() > 160 ||
        request.backendGeneration == 0 || request.observedAt <= 0 ||
        request.notBefore <= 0 || request.observedAt < request.notBefore)
    {
        result.status = NativeTimerModifyReadStatus::invalidRequest;
        return result;
    }

    RestfulApiNativeTimerInventoryReader strict(httpClient_);
    RestfulApiNativeTimerInventoryReadRequest snapshot;
    snapshot.backendId = request.backendId;
    snapshot.backendGeneration = request.backendGeneration;
    snapshot.observedAt = request.observedAt;
    const auto inventory = strict.read(snapshot);
    if (!inventory.ok())
        return result; // HTTP, truncation and parse failure are never success.

    const auto& ids = inventory.evidence.backendNativeTimerIds;
    if (std::count(ids.begin(), ids.end(), request.backendNativeTimerId) != 1)
    {
        result.status = NativeTimerModifyReadStatus::notFound;
        return result;
    }

    // A second direct GET must agree with the strict parser's *entire*
    // inventory identity set. No cached / stale or partial Timer vector
    // is allowed to confirm an otherwise uncertain native mutation.
    HttpRequest http;
    http.method = "GET";
    http.url = "/timers.json";
    http.headers["Accept"] = "application/json";
    http.headers["Cache-Control"] = "no-cache";
    const auto response = httpClient_.execute(http);
    if (response.statusCode != 200 ||
        response.body.size() > 4U * 1024U * 1024U)
        return result;
    const auto timers = RestfulApiTimerMapper::parseTimers(response.body);
    if (timers.size() != ids.size() || timers.size() > 4096U)
    {
        result.status = NativeTimerModifyReadStatus::inconsistent;
        return result;
    }
    const std::set<std::string> completeIds(ids.begin(), ids.end());
    std::set<std::string> mappedIds;
    for (const auto& timer : timers)
    {
        if (timer.id.empty() || !mappedIds.insert(timer.id).second)
        {
            result.status = NativeTimerModifyReadStatus::inconsistent;
            return result;
        }
    }
    if (mappedIds != completeIds)
    {
        result.status = NativeTimerModifyReadStatus::inconsistent;
        return result;
    }

    const auto it = std::find_if(
        timers.begin(), timers.end(), [&request](const VdrTimer& timer) {
            return timer.id == request.backendNativeTimerId;
        });
    if (it == timers.end())
    {
        result.status = NativeTimerModifyReadStatus::inconsistent;
        return result;
    }
    const auto mapped = VdrNativeTimerObservationMapper::map(
        request.backendId, request.backendGeneration,
        request.observedAt, *it);
    if (!mapped.ok())
    {
        result.status = NativeTimerModifyReadStatus::inconsistent;
        return result;
    }
    result.observation = mapped.observation;
    result.status = NativeTimerModifyReadStatus::observed;
    return result;
}
}

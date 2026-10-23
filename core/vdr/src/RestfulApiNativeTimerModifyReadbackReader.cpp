#include "RestfulApiNativeTimerModifyReadbackReader.h"

#include "RestfulApiNativeTimerInventoryReader.h"
#include "NativeTimerSpecification.h"
#include "RestfulApiTimerMapper.h"
#include "VdrNativeTimerObservationMapper.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <set>
#include <string>

namespace vdrsuite::vdr
{
namespace
{
// Diagnostic only: identify which direct-GET safety fence rejected an
// UPDATE readback without logging request contents or any credentials.
// Rate-limited even when a local provider is temporarily unavailable.
void logModifyReadbackFailure(
    const NativeTimerModifyReadRequest& request,
    const char* stage, int detail = 0)
{
    if (!request.allowIdentityChange) return;
    static std::int64_t nextLogAt = 0;
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (now < nextLogAt) return;
    nextLogAt = now + 60;
    std::cerr << "NATIVE_MODIFY_READER"
              << " stage=" << stage
              << " detail=" << detail
              << " backend=" << request.backendId << std::endl;
}
}
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
        request.notBefore <= 0 || request.observedAt < request.notBefore ||
        (request.allowIdentityChange &&
         !vdrsuite::timers::nativeTimerSpecificationValid(
             request.expectedSpecification)))
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
    {
        logModifyReadbackFailure(
            request, "strict_inventory", inventory.httpStatusCode != 0
                ? inventory.httpStatusCode
                : -1 - static_cast<int>(inventory.status));
        return result; // HTTP, truncation and parse failure are never success.
    }

    const auto& ids = inventory.evidence.backendNativeTimerIds;
    const bool oldIdPresent =
        std::count(ids.begin(), ids.end(), request.backendNativeTimerId) == 1;
    if (!oldIdPresent && !request.allowIdentityChange)
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
    {
        logModifyReadbackFailure(
            request, "second_inventory",
            response.statusCode == 200 ? -2 : response.statusCode);
        return result;
    }
    const auto timers = RestfulApiTimerMapper::parseTimers(response.body);
    if (timers.size() != ids.size() || timers.size() > 4096U)
    {
        logModifyReadbackFailure(
            request, "mapped_inventory_size",
            static_cast<int>(timers.size()) - static_cast<int>(ids.size()));
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
    if (it == timers.end() && request.allowIdentityChange &&
        !oldIdPresent)
    {
        // The native UPDATE may change the provider's compound Timer ID.
        // Only an EXACTLY ONE match for the requested postcondition in two
        // complete independently consistent inventories is acceptable.
        for (const auto& candidate : timers)
        {
            const auto observed = VdrNativeTimerObservationMapper::map(
                request.backendId, request.backendGeneration,
                request.observedAt, candidate);
            if (!observed.ok() ||
                !vdrsuite::timers::nativeTimerObservationMatchesSpecification(
                    request.expectedSpecification, observed.observation.observedState))
                continue;
            if (!result.observation.backendNativeTimerId.empty())
            {
                result.status = NativeTimerModifyReadStatus::inconsistent;
                return result;
            }
            result.observation = observed.observation;
        }
        if (result.observation.backendNativeTimerId.empty())
        {
            result.status = NativeTimerModifyReadStatus::notFound;
            return result;
        }
    }
    else
    {
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
    }
    result.status = NativeTimerModifyReadStatus::observed;
    return result;
}
}

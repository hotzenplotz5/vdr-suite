#pragma once

#include "NativeTimerInventoryEvidence.h"

#include <cstdint>
#include <string>

class IHttpClient;

namespace vdrsuite::vdr
{

enum class RestfulApiNativeTimerInventoryReadStatus
{
    complete,
    invalidRequest,
    httpError,
    parseError
};

struct RestfulApiNativeTimerInventoryReadRequest
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::int64_t observedAt = 0;
};

struct RestfulApiNativeTimerInventoryReadResult
{
    RestfulApiNativeTimerInventoryReadStatus status =
        RestfulApiNativeTimerInventoryReadStatus::parseError;
    int httpStatusCode = 0;
    vdrsuite::timers::NativeTimerInventoryEvidence evidence;

    bool ok() const
    {
        return status == RestfulApiNativeTimerInventoryReadStatus::complete;
    }
};

class RestfulApiNativeTimerInventoryReader
{
public:
    explicit RestfulApiNativeTimerInventoryReader(IHttpClient& httpClient);

    RestfulApiNativeTimerInventoryReadResult read(
        const RestfulApiNativeTimerInventoryReadRequest& request) const;

private:
    IHttpClient& httpClient_;
};

}

#pragma once

#include "NativeTimerObservation.h"

#include <cstdint>
#include <string>

class IHttpClient;

namespace vdrsuite::vdr
{
// One direct RESTfulAPI MODIFY readback. Unlike cached VdrSnapshots, a
// current HTTP 200 is required. An independent strict inventory and the
// field mapper must agree on EVERY native Timer ID, or verification fails.
enum class NativeTimerModifyReadStatus {
    observed, invalidRequest, unavailable, inconsistent, notFound
};
struct NativeTimerModifyReadRequest {
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::int64_t observedAt = 0;
    std::int64_t notBefore = 0;
};
struct NativeTimerModifyReadResult {
    NativeTimerModifyReadStatus status = NativeTimerModifyReadStatus::unavailable;
    vdrsuite::timers::NativeTimerObservation observation;
    bool ok() const { return status == NativeTimerModifyReadStatus::observed; }
};
class RestfulApiNativeTimerModifyReadbackReader {
public:
    explicit RestfulApiNativeTimerModifyReadbackReader(IHttpClient& httpClient);
    NativeTimerModifyReadResult read(const NativeTimerModifyReadRequest& request) const;
private:
    IHttpClient& httpClient_;
};
}

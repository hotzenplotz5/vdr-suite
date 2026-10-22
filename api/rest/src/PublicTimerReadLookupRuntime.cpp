#include "PublicApiRuntime.h"

#include <mutex>
#include <utility>

// NativeTimer read callback lifecycle; the original mutexes and callback
// slots remain on the single PublicApiRuntime instance.
void PublicApiRuntime::registerNativeTimersLookup(
    NativeTimersLookup lookup)
{
    std::lock_guard<std::mutex> lock(nativeTimersLookupMutex_);
    nativeTimersLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetNativeTimersLookup()
{
    std::lock_guard<std::mutex> lock(nativeTimersLookupMutex_);
    nativeTimersLookup_ = {};
}

bool PublicApiRuntime::nativeTimersLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(nativeTimersLookupMutex_);
    return static_cast<bool>(nativeTimersLookup_);
}

PublicNativeTimersResult PublicApiRuntime::lookupNativeTimers(
    const PublicNativeTimersRequest& request) const
{
    NativeTimersLookup lookup;
    {
        std::lock_guard<std::mutex> lock(nativeTimersLookupMutex_);
        lookup = nativeTimersLookup_;
    }
    return lookup ? lookup(request) : PublicNativeTimersResult{};
}

void PublicApiRuntime::registerNativeTimerDetailLookup(NativeTimerDetailLookup cb)
{
    std::lock_guard<std::mutex> lock(nativeTimerDetailMutex_);
    nativeTimerDetailLookup_ = std::move(cb);
}

void PublicApiRuntime::resetNativeTimerDetailLookup()
{
    std::lock_guard<std::mutex> lock(nativeTimerDetailMutex_);
    nativeTimerDetailLookup_ = {};
}

PublicNativeTimerDetailResult PublicApiRuntime::lookupNativeTimerDetail(
    const PublicNativeTimerDetailRequest& request) const
{
    NativeTimerDetailLookup cb;
    {
        std::lock_guard<std::mutex> lock(nativeTimerDetailMutex_);
        cb = nativeTimerDetailLookup_;
    }
    return cb ? cb(request) : PublicNativeTimerDetailResult{};
}


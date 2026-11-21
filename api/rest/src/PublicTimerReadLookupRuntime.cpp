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


// TimerAssignment and SearchTimer read-only callbacks retain their
// original lock and executor-free invocation semantics.
void PublicApiRuntime::registerTimerAssignmentLookup(
    TimerAssignmentLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    timerAssignmentLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetTimerAssignmentLookup()
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    timerAssignmentLookup_ = TimerAssignmentLookup{};
}

bool PublicApiRuntime::timerAssignmentLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    return static_cast<bool>(timerAssignmentLookup_);
}

void PublicApiRuntime::registerTimerAssignmentCollectionLookup(
    TimerAssignmentCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentCollectionLookupMutex_);
    timerAssignmentCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetTimerAssignmentCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentCollectionLookupMutex_);
    timerAssignmentCollectionLookup_ = {};
}

bool PublicApiRuntime::timerAssignmentCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentCollectionLookupMutex_);
    return static_cast<bool>(timerAssignmentCollectionLookup_);
}

PublicTimerAssignmentCollectionResult
PublicApiRuntime::lookupTimerAssignmentCollection(
    const PublicTimerAssignmentCollectionRequest& request) const
{
    TimerAssignmentCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            timerAssignmentCollectionLookupMutex_);
        lookup = timerAssignmentCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}

void PublicApiRuntime::registerSearchTimerCollectionLookup(
    SearchTimerCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(searchTimerCollectionLookupMutex_);
    searchTimerCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetSearchTimerCollectionLookup()
{
    std::lock_guard<std::mutex> lock(searchTimerCollectionLookupMutex_);
    searchTimerCollectionLookup_ = {};
}

bool PublicApiRuntime::searchTimerCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(searchTimerCollectionLookupMutex_);
    return static_cast<bool>(searchTimerCollectionLookup_);
}

PublicSearchTimerCollectionResult PublicApiRuntime::lookupSearchTimerCollection(
    const PublicSearchTimerCollectionRequest& request) const
{
    SearchTimerCollectionLookup callback;
    {
        std::lock_guard<std::mutex> lock(searchTimerCollectionLookupMutex_);
        callback = searchTimerCollectionLookup_;
    }
    return callback ? callback(request) : PublicSearchTimerCollectionResult{};
}

void PublicApiRuntime::registerSearchTimerPreviewLookup(
    SearchTimerPreviewLookup lookup)
{
    std::lock_guard<std::mutex> lock(searchTimerPreviewLookupMutex_);
    searchTimerPreviewLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetSearchTimerPreviewLookup()
{
    std::lock_guard<std::mutex> lock(searchTimerPreviewLookupMutex_);
    searchTimerPreviewLookup_ = {};
}

PublicSearchTimerPreviewResult PublicApiRuntime::lookupSearchTimerPreview(
    const PublicSearchTimerPreviewRequest& request) const
{
    SearchTimerPreviewLookup callback;
    {
        std::lock_guard<std::mutex> lock(searchTimerPreviewLookupMutex_);
        callback = searchTimerPreviewLookup_;
    }
    return callback ? callback(request) : PublicSearchTimerPreviewResult{};
}

PublicTimerAssignmentLookupResult
PublicApiRuntime::lookupTimerAssignment(
    const std::string& timerAssignmentId,
    const std::string& backendId) const
{
    TimerAssignmentLookup lookup;

    {
        std::lock_guard<std::mutex> lock(
            timerAssignmentLookupMutex_);
        lookup = timerAssignmentLookup_;
    }

    if (!lookup)
    {
        return {};
    }

    return lookup(timerAssignmentId, backendId);
}


#include "PublicApiRuntime.h"

#include <mutex>
#include <utility>

// Existing EPG read callback lifecycle on the original PublicApiRuntime
// instance and mutexes; no new lookup, fallback or synchronization.
void PublicApiRuntime::registerEpgNowNextLookup(
    EpgNowNextLookup lookup)
{
    std::lock_guard<std::mutex> lock(epgNowNextLookupMutex_);
    epgNowNextLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetEpgNowNextLookup()
{
    std::lock_guard<std::mutex> lock(epgNowNextLookupMutex_);
    epgNowNextLookup_ = {};
}

bool PublicApiRuntime::epgNowNextLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(epgNowNextLookupMutex_);
    return static_cast<bool>(epgNowNextLookup_);
}

PublicEpgNowNextResult PublicApiRuntime::lookupEpgNowNext(
    const PublicEpgNowNextRequest& request) const
{
    EpgNowNextLookup lookup;
    {
        std::lock_guard<std::mutex> lock(epgNowNextLookupMutex_);
        lookup = epgNowNextLookup_;
    }
    return lookup ? lookup(request) : PublicEpgNowNextResult{};
}

void PublicApiRuntime::registerEpgWindowLookup(
    EpgWindowLookup lookup)
{
    std::lock_guard<std::mutex> lock(epgWindowLookupMutex_);
    epgWindowLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetEpgWindowLookup()
{
    std::lock_guard<std::mutex> lock(epgWindowLookupMutex_);
    epgWindowLookup_ = {};
}

bool PublicApiRuntime::epgWindowLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(epgWindowLookupMutex_);
    return static_cast<bool>(epgWindowLookup_);
}

PublicEpgWindowResult PublicApiRuntime::lookupEpgWindow(
    const PublicEpgWindowRequest& request) const
{
    EpgWindowLookup lookup;
    {
        std::lock_guard<std::mutex> lock(epgWindowLookupMutex_);
        lookup = epgWindowLookup_;
    }
    return lookup ? lookup(request) : PublicEpgWindowResult{};
}

void PublicApiRuntime::registerEpgEventDetailLookup(EpgEventDetailLookup callback)
{
    std::lock_guard<std::mutex> lock(epgEventDetailMutex_);
    epgEventDetailLookup_ = std::move(callback);
}
void PublicApiRuntime::resetEpgEventDetailLookup()
{
    std::lock_guard<std::mutex> lock(epgEventDetailMutex_);
    epgEventDetailLookup_ = {};
}
bool PublicApiRuntime::epgEventDetailLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(epgEventDetailMutex_);
    return static_cast<bool>(epgEventDetailLookup_);
}
PublicEpgEventDetailResult PublicApiRuntime::lookupEpgEventDetail(
    const PublicEpgEventDetailRequest& request) const
{
    EpgEventDetailLookup callback;
    {
        std::lock_guard<std::mutex> lock(epgEventDetailMutex_);
        callback = epgEventDetailLookup_;
    }
    return callback ? callback(request) : PublicEpgEventDetailResult{};
}


#include "PublicApiRuntime.h"

#include <mutex>
#include <utility>

// Channel callback registration, lookup and reset use the original
// PublicApiRuntime mutex and callback slot.
void PublicApiRuntime::registerChannelCollectionLookup(
    ChannelCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        channelCollectionLookupMutex_);
    channelCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetChannelCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        channelCollectionLookupMutex_);
    channelCollectionLookup_ = {};
}

bool PublicApiRuntime::channelCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        channelCollectionLookupMutex_);
    return static_cast<bool>(channelCollectionLookup_);
}

PublicChannelCollectionResult
PublicApiRuntime::lookupChannelCollection(
    const PublicChannelCollectionRequest& request) const
{
    ChannelCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            channelCollectionLookupMutex_);
        lookup = channelCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}


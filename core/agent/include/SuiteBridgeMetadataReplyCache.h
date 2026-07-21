#pragma once

#include "ISuiteBridgeMetadataTransport.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <utility>

namespace vdrsuite::agent
{

class SuiteBridgeMetadataReplyCache
{
public:
    static constexpr std::size_t DefaultMaximumEntries = 128;
    static constexpr std::chrono::milliseconds DefaultTtl{300000};

    SuiteBridgeMetadataReplyCache(
        std::size_t maximumEntries = DefaultMaximumEntries,
        std::chrono::milliseconds ttl = DefaultTtl);

    bool find(
        const std::string& channelId,
        const std::string& eventId,
        SuiteBridgeMetadataCommandReply& reply);

    void store(
        const std::string& channelId,
        const std::string& eventId,
        const SuiteBridgeMetadataCommandReply& reply);

    std::size_t size() const;

private:
    using Key = std::pair<std::string, std::string>;
    using Clock = std::chrono::steady_clock;

    struct Entry
    {
        SuiteBridgeMetadataCommandReply reply;
        Clock::time_point expiresAt;
        std::uint64_t sequence = 0;
    };

    void removeExpiredLocked(Clock::time_point now);
    void removeOldestLocked();

    const std::size_t maximumEntries_;
    const std::chrono::milliseconds ttl_;
    mutable std::mutex mutex_;
    std::map<Key, Entry> entries_;
    std::uint64_t sequence_ = 0;
};

}

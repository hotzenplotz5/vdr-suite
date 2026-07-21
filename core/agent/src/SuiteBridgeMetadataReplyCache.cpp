#include "SuiteBridgeMetadataReplyCache.h"

#include <algorithm>
#include <utility>

namespace vdrsuite::agent
{

SuiteBridgeMetadataReplyCache::SuiteBridgeMetadataReplyCache(
    std::size_t maximumEntries,
    std::chrono::milliseconds ttl)
    : maximumEntries_(maximumEntries),
      ttl_(ttl)
{
}

bool SuiteBridgeMetadataReplyCache::find(
    const std::string& channelId,
    const std::string& eventId,
    SuiteBridgeMetadataCommandReply& reply)
{
    if (maximumEntries_ == 0 || ttl_.count() <= 0)
    {
        return false;
    }

    const Clock::time_point now = Clock::now();
    const Key key{channelId, eventId};

    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = entries_.find(key);
    if (iterator == entries_.end())
    {
        return false;
    }

    if (iterator->second.expiresAt <= now)
    {
        entries_.erase(iterator);
        return false;
    }

    iterator->second.sequence = ++sequence_;
    reply = iterator->second.reply;
    return true;
}

void SuiteBridgeMetadataReplyCache::store(
    const std::string& channelId,
    const std::string& eventId,
    const SuiteBridgeMetadataCommandReply& reply)
{
    if (maximumEntries_ == 0 ||
        ttl_.count() <= 0 ||
        !reply.transportSucceeded ||
        reply.replyCode != 250)
    {
        return;
    }

    const Clock::time_point now = Clock::now();
    const Key key{channelId, eventId};

    std::lock_guard<std::mutex> lock(mutex_);
    removeExpiredLocked(now);

    const auto iterator = entries_.find(key);
    if (iterator == entries_.end() &&
        entries_.size() >= maximumEntries_)
    {
        removeOldestLocked();
    }

    Entry entry;
    entry.reply = reply;
    entry.expiresAt = now + ttl_;
    entry.sequence = ++sequence_;
    entries_[key] = std::move(entry);
}

std::size_t SuiteBridgeMetadataReplyCache::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void SuiteBridgeMetadataReplyCache::removeExpiredLocked(
    Clock::time_point now)
{
    for (auto iterator = entries_.begin(); iterator != entries_.end();)
    {
        if (iterator->second.expiresAt <= now)
        {
            iterator = entries_.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
}

void SuiteBridgeMetadataReplyCache::removeOldestLocked()
{
    if (entries_.empty())
    {
        return;
    }

    const auto oldest = std::min_element(
        entries_.begin(),
        entries_.end(),
        [](const auto& left, const auto& right)
        {
            return left.second.sequence < right.second.sequence;
        });

    entries_.erase(oldest);
}

}

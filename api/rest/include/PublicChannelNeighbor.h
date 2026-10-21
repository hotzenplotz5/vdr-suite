#pragma once

// Pure, read-only VDR-order adjacent channel projection.
// Works on an already authorized full snapshot. It owns no channel inventory,
// credentials, transport URL, playback or MediaSession.
#include "PublicApiRuntime.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

inline std::optional<PublicChannelCollectionItem> findPublicVdrChannelNeighbor(
    const std::vector<PublicChannelCollectionItem>& channels,
    const std::string& currentChannelId,
    const std::string& direction)
{
    if (currentChannelId.empty() ||
        (direction != "up" && direction != "down"))
        return std::nullopt;

    std::vector<const PublicChannelCollectionItem*> eligible;
    eligible.reserve(channels.size());
    for (const auto& channel : channels)
    {
        if (channel.enabled && !channel.radio && channel.channelNumber > 0)
            eligible.push_back(&channel);
    }
    if (eligible.size() < 2U) return std::nullopt;

    std::sort(eligible.begin(), eligible.end(),
        [](const PublicChannelCollectionItem* left,
           const PublicChannelCollectionItem* right)
        {
            if (left->channelNumber != right->channelNumber)
                return left->channelNumber < right->channelNumber;
            return left->channelId < right->channelId;
        });

    const auto current = std::find_if(
        eligible.begin(), eligible.end(),
        [&currentChannelId](const PublicChannelCollectionItem* candidate)
        {
            return candidate->channelId == currentChannelId;
        });
    if (current == eligible.end()) return std::nullopt;

    const std::size_t index =
        static_cast<std::size_t>(current - eligible.begin());
    const std::size_t next = direction == "up" ?
        (index + 1U) % eligible.size() :
        (index + eligible.size() - 1U) % eligible.size();
    return *eligible[next];
}

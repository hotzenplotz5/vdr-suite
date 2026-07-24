#pragma once

#include "EpgScraperMetadata.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct SuiteBridgeEpgTypeSnapshotTransportItem
{
    std::string channelId;
    std::string eventId;
    std::int64_t startTime = 0;
    std::int64_t endTime = 0;
    EpgScraperMediaType mediaType = EpgScraperMediaType::None;
};

struct SuiteBridgeEpgTypeSnapshotTransportPage
{
    bool transportSucceeded = false;
    bool payloadValid = false;
    int replyCode = 0;
    std::uint64_t nextOffset = 0;
    std::size_t scanned = 0;
    bool done = true;
    std::vector<SuiteBridgeEpgTypeSnapshotTransportItem> items;
};

class ISuiteBridgeEpgTypeSnapshotTransport
{
public:
    virtual ~ISuiteBridgeEpgTypeSnapshotTransport() = default;

    virtual SuiteBridgeEpgTypeSnapshotTransportPage requestEpgTypeSnapshot(
        std::int64_t fromTime,
        std::int64_t untilTime,
        std::uint64_t offset,
        std::size_t limit) = 0;
};

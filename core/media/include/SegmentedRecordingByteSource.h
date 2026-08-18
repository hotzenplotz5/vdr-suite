#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct RecordingByteExtent
{
    std::uint64_t readableBytes = 0;
    std::size_t segmentCount = 0;
    bool growing = false;
};

struct RecordingByteReadResult
{
    bool success = false;
    std::string reasonCode;
    std::vector<unsigned char> bytes;
    std::uint64_t nextOffset = 0;
    bool endOfReadableExtent = false;
    RecordingByteExtent extent;
};

class SegmentedRecordingByteSource
{
public:
    using SegmentCatalog = std::function<std::vector<std::string>()>;

    static constexpr std::size_t DefaultMaximumReadBytes = 512 * 1024;

    SegmentedRecordingByteSource(
        SegmentCatalog segmentCatalog,
        bool growing,
        std::size_t maximumReadBytes = DefaultMaximumReadBytes);

    RecordingByteExtent refreshExtent() const;

    RecordingByteReadResult read(
        std::uint64_t offset,
        std::size_t requestedBytes) const;

private:
    struct Segment
    {
        std::string path;
        std::uint64_t start = 0;
        std::uint64_t end = 0;
    };

    struct Snapshot
    {
        bool valid = false;
        std::string reasonCode;
        RecordingByteExtent extent;
        std::vector<Segment> segments;
    };

    Snapshot snapshot() const;

    SegmentCatalog segmentCatalog_;
    bool growing_ = false;
    std::size_t maximumReadBytes_ = DefaultMaximumReadBytes;
};
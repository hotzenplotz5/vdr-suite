#include "SegmentedRecordingByteSource.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

SegmentedRecordingByteSource::SegmentedRecordingByteSource(
    SegmentCatalog segmentCatalog,
    bool growing,
    std::size_t maximumReadBytes)
    : segmentCatalog_(std::move(segmentCatalog)),
      growing_(growing),
      maximumReadBytes_(maximumReadBytes == 0
          ? DefaultMaximumReadBytes
          : maximumReadBytes)
{
}

SegmentedRecordingByteSource::Snapshot
SegmentedRecordingByteSource::snapshot() const
{
    Snapshot result;
    result.extent.growing = growing_;

    if (!segmentCatalog_) {
        result.reasonCode = "recording_segment_catalog_unavailable";
        return result;
    }

    std::vector<std::string> paths;
    try {
        paths = segmentCatalog_();
    }
    catch (...) {
        result.reasonCode = "recording_segment_catalog_failed";
        return result;
    }

    if (paths.empty()) {
        result.reasonCode = "recording_segments_unavailable";
        return result;
    }

    std::uint64_t offset = 0;
    result.segments.reserve(paths.size());

    for (const std::string& path : paths) {
        struct stat metadata{};
        if (path.empty() || lstat(path.c_str(), &metadata) != 0) {
            result.reasonCode = "recording_segment_stat_failed";
            return result;
        }

        // Provider catalog entries are trusted VDR recording segments, but
        // reject symlinks and non-regular files so a later filesystem change
        // cannot turn a lease into an arbitrary file reader.
        if (!S_ISREG(metadata.st_mode) || metadata.st_size < 0) {
            result.reasonCode = "recording_segment_not_regular";
            return result;
        }

        const std::uint64_t size = static_cast<std::uint64_t>(metadata.st_size);
        if (size > std::numeric_limits<std::uint64_t>::max() - offset) {
            result.reasonCode = "recording_extent_overflow";
            return result;
        }

        Segment segment;
        segment.path = path;
        segment.start = offset;
        segment.end = offset + size;
        result.segments.push_back(std::move(segment));
        offset += size;
    }

    result.extent.readableBytes = offset;
    result.extent.segmentCount = result.segments.size();
    result.valid = true;
    return result;
}

RecordingByteExtent SegmentedRecordingByteSource::refreshExtent() const
{
    return snapshot().extent;
}

RecordingByteReadResult SegmentedRecordingByteSource::read(
    std::uint64_t offset,
    std::size_t requestedBytes) const
{
    RecordingByteReadResult result;
    const Snapshot current = snapshot();
    result.extent = current.extent;
    result.nextOffset = offset;

    if (!current.valid) {
        result.reasonCode = current.reasonCode;
        return result;
    }

    if (offset > current.extent.readableBytes) {
        result.reasonCode = "recording_range_not_satisfiable";
        return result;
    }

    if (requestedBytes == 0 || offset == current.extent.readableBytes) {
        result.success = true;
        result.endOfReadableExtent = offset == current.extent.readableBytes;
        return result;
    }

    const std::uint64_t readable = current.extent.readableBytes - offset;
    const std::size_t boundedRequest = std::min(
        requestedBytes,
        maximumReadBytes_);
    const std::size_t wanted = static_cast<std::size_t>(
        std::min<std::uint64_t>(
            readable,
            static_cast<std::uint64_t>(boundedRequest)));

    result.bytes.reserve(wanted);
    std::uint64_t cursor = offset;
    std::size_t remaining = wanted;

    for (const Segment& segment : current.segments) {
        if (remaining == 0) {
            break;
        }
        if (segment.end <= cursor || segment.end == segment.start) {
            continue;
        }
        if (cursor < segment.start) {
            result.reasonCode = "recording_segment_map_gap";
            result.bytes.clear();
            return result;
        }

        const std::uint64_t localOffset = cursor - segment.start;
        const std::uint64_t segmentRemaining = segment.end - cursor;
        const std::size_t chunk = static_cast<std::size_t>(
            std::min<std::uint64_t>(
                segmentRemaining,
                static_cast<std::uint64_t>(remaining)));

        const int fd = open(
            segment.path.c_str(),
            O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) {
            result.reasonCode = "recording_segment_open_failed";
            result.bytes.clear();
            return result;
        }

        const std::size_t oldSize = result.bytes.size();
        result.bytes.resize(oldSize + chunk);
        std::size_t chunkRead = 0;

        while (chunkRead < chunk) {
            const ssize_t count = pread(
                fd,
                result.bytes.data() + oldSize + chunkRead,
                chunk - chunkRead,
                static_cast<off_t>(localOffset + chunkRead));

            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                break;
            }
            chunkRead += static_cast<std::size_t>(count);
        }

        close(fd);
        result.bytes.resize(oldSize + chunkRead);

        if (chunkRead == 0) {
            result.reasonCode = "recording_segment_read_failed";
            result.bytes.clear();
            return result;
        }

        cursor += chunkRead;
        remaining -= chunkRead;

        if (chunkRead < chunk) {
            break;
        }
    }

    if (result.bytes.empty()) {
        result.reasonCode = "recording_read_empty";
        return result;
    }

    result.success = true;
    result.nextOffset = offset + result.bytes.size();
    result.endOfReadableExtent =
        result.nextOffset >= current.extent.readableBytes;
    return result;
}
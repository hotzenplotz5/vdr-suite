#pragma once

#include "SegmentedRecordingByteSource.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct RecordingDirectSourceRegistration
{
    std::string recordingDirectory;
    std::vector<std::string> segmentPaths;
    std::string sourceFingerprint;
    std::uint64_t readableBytes = 0;
};

struct RecordingDirectSourceLookup
{
    bool available = false;
    std::string reasonCode;
    std::uint64_t readableBytes = 0;
};

class RecordingDirectSourceRegistry
{
public:
    bool registerCompleted(
        const std::string& sessionId,
        const RecordingDirectSourceRegistration& registration,
        std::string& reasonCode);

    RecordingDirectSourceLookup lookup(
        const std::string& sessionId) const;

    RecordingByteReadResult read(
        const std::string& sessionId,
        std::uint64_t offset,
        std::size_t requestedBytes) const;

    void remove(const std::string& sessionId);

private:
    struct Entry
    {
        std::string recordingDirectory;
        std::vector<std::string> segmentPaths;
        std::string sourceFingerprint;
        std::uint64_t readableBytes = 0;
    };

    static RecordingDirectSourceLookup validate(const Entry& entry);

    mutable std::mutex mutex_;
    std::map<std::string, Entry> entries_;
};

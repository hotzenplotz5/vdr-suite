#include "RecordingDirectSourceRegistry.h"

#include "RecordingSourceFingerprint.h"

#include <utility>

namespace
{

RecordingDirectSourceLookup unavailable(const std::string& reasonCode)
{
    RecordingDirectSourceLookup result;
    result.reasonCode = reasonCode;
    return result;
}

RecordingByteReadResult readFailure(const std::string& reasonCode)
{
    RecordingByteReadResult result;
    result.reasonCode = reasonCode;
    return result;
}

} // namespace

RecordingDirectSourceLookup RecordingDirectSourceRegistry::validate(
    const Entry& entry)
{
    const RecordingSourceFingerprint current = inspectRecordingSource(
        entry.recordingDirectory,
        entry.segmentPaths);
    if (!current.valid) {
        return unavailable(
            current.reasonCode.empty()
                ? "recording_source_unavailable"
                : current.reasonCode);
    }
    if (current.growing) {
        return unavailable("recording_source_is_growing");
    }
    if (current.value != entry.sourceFingerprint ||
        current.readableBytes != entry.readableBytes) {
        return unavailable("recording_source_changed");
    }

    RecordingDirectSourceLookup result;
    result.available = true;
    result.readableBytes = current.readableBytes;
    return result;
}

bool RecordingDirectSourceRegistry::registerCompleted(
    const std::string& sessionId,
    const RecordingDirectSourceRegistration& registration,
    std::string& reasonCode)
{
    reasonCode.clear();
    if (sessionId.empty() || registration.recordingDirectory.empty() ||
        registration.segmentPaths.empty() || registration.sourceFingerprint.empty()) {
        reasonCode = "invalid_recording_direct_source";
        return false;
    }

    Entry entry;
    entry.recordingDirectory = registration.recordingDirectory;
    entry.segmentPaths = registration.segmentPaths;
    entry.sourceFingerprint = registration.sourceFingerprint;
    entry.readableBytes = registration.readableBytes;

    const RecordingDirectSourceLookup current = validate(entry);
    if (!current.available) {
        reasonCode = current.reasonCode.empty()
            ? "recording_direct_source_unavailable"
            : current.reasonCode;
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.find(sessionId) != entries_.end()) {
        reasonCode = "media_session_already_owned";
        return false;
    }
    entries_.emplace(sessionId, std::move(entry));
    return true;
}

RecordingDirectSourceLookup RecordingDirectSourceRegistry::lookup(
    const std::string& sessionId) const
{
    Entry entry;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = entries_.find(sessionId);
        if (found == entries_.end()) {
            return unavailable("recording_direct_source_not_registered");
        }
        entry = found->second;
    }
    return validate(entry);
}

RecordingByteReadResult RecordingDirectSourceRegistry::read(
    const std::string& sessionId,
    std::uint64_t offset,
    std::size_t requestedBytes) const
{
    Entry entry;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = entries_.find(sessionId);
        if (found == entries_.end()) {
            return readFailure("recording_direct_source_not_registered");
        }
        entry = found->second;
    }

    const RecordingDirectSourceLookup before = validate(entry);
    if (!before.available) {
        return readFailure(before.reasonCode);
    }

    const std::vector<std::string> segmentPaths = entry.segmentPaths;
    SegmentedRecordingByteSource source(
        [segmentPaths]() { return segmentPaths; },
        false);
    RecordingByteReadResult result = source.read(offset, requestedBytes);
    if (!result.success) {
        return result;
    }

    const RecordingDirectSourceLookup after = validate(entry);
    if (!after.available) {
        return readFailure(after.reasonCode);
    }
    result.extent.readableBytes = after.readableBytes;
    result.extent.growing = false;
    return result;
}

void RecordingDirectSourceRegistry::remove(const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.erase(sessionId);
}

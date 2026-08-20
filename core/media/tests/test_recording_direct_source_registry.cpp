#include "RecordingDirectSourceRegistry.h"

#include "RecordingSourceFingerprint.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& value)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
}

std::string asString(const std::vector<unsigned char>& bytes)
{
    return std::string(bytes.begin(), bytes.end());
}

} // namespace

int main()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-recording-direct-source-test-" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto first = root / "00001.ts";
    const auto second = root / "00002.ts";
    writeFile(first, "ABCDE");
    writeFile(second, "FGHIJ");
    const std::vector<std::string> segments = {first.string(), second.string()};

    const RecordingSourceFingerprint fingerprint =
        inspectRecordingSource(root.string(), segments);
    assert(fingerprint.valid);
    assert(!fingerprint.growing);
    assert(fingerprint.readableBytes == 10);

    RecordingDirectSourceRegistry registry;
    RecordingDirectSourceRegistration registration;
    registration.recordingDirectory = root.string();
    registration.segmentPaths = segments;
    registration.sourceFingerprint = fingerprint.value;
    registration.readableBytes = fingerprint.readableBytes;
    std::string reason;
    assert(registry.registerCompleted("session-1", registration, reason));
    assert(reason.empty());

    const auto lookup = registry.lookup("session-1");
    assert(lookup.available);
    assert(lookup.readableBytes == 10);

    const auto crossSegment = registry.read("session-1", 3, 6);
    assert(crossSegment.success);
    assert(asString(crossSegment.bytes) == "DEFGHI");
    assert(crossSegment.nextOffset == 9);

    registry.remove("session-1");
    assert(!registry.lookup("session-1").available);

    const std::string large(
        SegmentedRecordingByteSource::DefaultMaximumReadBytes + 1024,
        'x');
    writeFile(first, large);
    writeFile(second, "Y");
    const RecordingSourceFingerprint largeFingerprint =
        inspectRecordingSource(root.string(), segments);
    assert(largeFingerprint.valid);
    registration.sourceFingerprint = largeFingerprint.value;
    registration.readableBytes = largeFingerprint.readableBytes;
    assert(registry.registerCompleted("session-large", registration, reason));
    const auto bounded = registry.read(
        "session-large",
        0,
        SegmentedRecordingByteSource::DefaultMaximumReadBytes * 4);
    assert(bounded.success);
    assert(bounded.bytes.size() ==
        SegmentedRecordingByteSource::DefaultMaximumReadBytes);
    registry.remove("session-large");

    const RecordingSourceFingerprint stableFingerprint =
        inspectRecordingSource(root.string(), segments);
    registration.sourceFingerprint = stableFingerprint.value;
    registration.readableBytes = stableFingerprint.readableBytes;
    assert(registry.registerCompleted("session-drift", registration, reason));
    writeFile(second, "changed");
    const auto drifted = registry.lookup("session-drift");
    assert(!drifted.available);
    assert(drifted.reasonCode == "recording_source_changed");
    registry.remove("session-drift");

    const RecordingSourceFingerprint completedFingerprint =
        inspectRecordingSource(root.string(), segments);
    registration.sourceFingerprint = completedFingerprint.value;
    registration.readableBytes = completedFingerprint.readableBytes;
    assert(registry.registerCompleted("session-growing", registration, reason));
    writeFile(root / ".timer", "active");
    const auto nowGrowing = registry.lookup("session-growing");
    assert(!nowGrowing.available);
    assert(nowGrowing.reasonCode == "recording_source_is_growing");
    registry.remove("session-growing");

    std::filesystem::remove_all(root);
    return 0;
}

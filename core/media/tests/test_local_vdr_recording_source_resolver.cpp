#include "LocalVdrRecordingSourceResolver.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{

std::filesystem::path makeTempRecordingDirectory()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        "vdr-suite-phase65-recording-source-test";
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directories(root);
    return root;
}

void touch(const std::filesystem::path& path)
{
    std::ofstream stream(path.string(), std::ios::binary);
    stream << "segment";
}

} // namespace

int main()
{
    const std::filesystem::path directory = makeTempRecordingDirectory();
    touch(directory / "00002.ts");
    touch(directory / "00001.ts");
    touch(directory / "index");
    touch(directory / "info");

    VdrRecording recording;
    recording.id = "42";
    recording.backendId = "default";
    recording.backendNativeId = directory.string();

    LocalVdrRecordingSourceResolver resolver(
        [recording](const std::string& backendId) {
            if (backendId == "default") {
                return std::vector<VdrRecording>{recording};
            }
            return std::vector<VdrRecording>{};
        });

    const auto resolved = resolver.resolve("default", "42");
    assert(resolved.resolved);
    assert(resolved.reasonCode.empty());
    assert(resolved.source.backendId == "default");
    assert(resolved.source.recordingId == "42");
    assert(resolved.source.segmentPaths.size() == 2);
    assert(std::filesystem::path(resolved.source.segmentPaths[0]).filename() == "00001.ts");
    assert(std::filesystem::path(resolved.source.segmentPaths[1]).filename() == "00002.ts");
    assert(!resolved.source.growing);
    assert(resolved.source.progressiveDirectSafe);
    assert(resolved.source.readableBytes == 14);
    assert(!resolved.source.sourceFingerprint.empty());

    touch(directory / ".timer");
    const auto growing = resolver.resolve("default", "42");
    assert(growing.resolved);
    assert(growing.source.growing);
    assert(growing.source.progressiveDirectSafe);
    assert(growing.source.readableBytes == 14);
    assert(growing.source.sourceFingerprint != resolved.source.sourceFingerprint);
    std::filesystem::remove(directory / ".timer");

    const auto missing = resolver.resolve("default", "99");
    assert(!missing.resolved);
    assert(missing.reasonCode == "recording_not_found");

    VdrRecording relative;
    relative.id = "7";
    relative.backendNativeId = "relative/path.rec";
    LocalVdrRecordingSourceResolver relativeResolver(
        [relative](const std::string&) {
            return std::vector<VdrRecording>{relative};
        });
    const auto unsafe = relativeResolver.resolve("default", "7");
    assert(!unsafe.resolved);
    assert(unsafe.reasonCode == "recording_internal_path_unavailable");

    const std::filesystem::path legacyDirectory = directory / "legacy.rec";
    std::filesystem::create_directories(legacyDirectory);
    touch(legacyDirectory / "001.vdr");
    VdrRecording legacy;
    legacy.id = "legacy";
    legacy.backendNativeId = legacyDirectory.string();
    LocalVdrRecordingSourceResolver legacyResolver(
        [legacy](const std::string&) {
            return std::vector<VdrRecording>{legacy};
        });
    const auto legacyResolved = legacyResolver.resolve("default", "legacy");
    assert(legacyResolved.resolved);
    assert(!legacyResolved.source.growing);
    assert(!legacyResolved.source.progressiveDirectSafe);

    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    return 0;
}

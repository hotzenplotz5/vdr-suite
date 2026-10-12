#include "DeviceRecordingResumeProjection.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
void writeIndex(
    const std::filesystem::path& path,
    int firstNumber,
    int secondNumber)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    assert(stream.good());
    for (const int number : {firstNumber, secondNumber}) {
        for (int frame = 0; frame < 75; ++frame) {
            std::array<unsigned char, 8> entry{};
            entry[6] = static_cast<unsigned char>(number & 0xff);
            entry[7] = static_cast<unsigned char>((number >> 8) & 0xff);
            stream.write(reinterpret_cast<const char*>(entry.data()),
                         static_cast<std::streamsize>(entry.size()));
        }
    }
    assert(stream.good());
}

void writeSegment(const std::filesystem::path& path)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out.good());
    std::array<char, 188> packet{};
    out.write(packet.data(), static_cast<std::streamsize>(packet.size()));
    assert(out.good());
}
}

int main()
{
    const auto directory = std::filesystem::temp_directory_path() /
        "vdr-suite-device-resume-read-projection-test.rec";
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    std::filesystem::create_directories(directory);
    {
        std::ofstream info(directory / "info");
        info << "F 25 1920 1080 p 16:9\n";
    }
    writeSegment(directory / "00001.ts");
    writeSegment(directory / "00002.ts");
    writeIndex(directory / "index", 1, 2);
    VdrRecording recording;
    recording.id = "internal-native-id";
    recording.backendId = "default";
    recording.backendNativeId = directory.string();
    LocalVdrRecordingSourceResolution source;
    source.resolved = true;
    source.source.segmentPaths = {
        (directory / "00001.ts").string(),
        (directory / "00002.ts").string()};
    source.source.growing = false;

    auto ready = projectDeviceRecordingResume(recording, source,
                                              "hls-fmp4", false);
    assert(ready.durationSeconds == 6);
    assert(ready.supported);
    assert(!ready.preparing);

    auto preparing = projectDeviceRecordingResume(recording, source,
                                                  "hls-fmp4", true);
    assert(preparing.durationSeconds == 6);
    assert(!preparing.supported);
    assert(preparing.preparing);

    auto progressive = projectDeviceRecordingResume(recording, source,
                                                    "progressive-direct", false);
    assert(progressive.durationSeconds == 6);
    assert(!progressive.supported);

    source.source.growing = true;
    assert(!projectDeviceRecordingResume(recording, source,
                                          "hls-fmp4", false).supported);
    source.source.growing = false;

    source.resolved = false;
    assert(!projectDeviceRecordingResume(recording, source,
                                          "hls-fmp4", false).supported);
    source.resolved = true;

    // A valid duration with an unmapped middle TS segment is not
    // recording-wide seekability. This reproduces the exact UI ambiguity:
    // "01:23:xx" can be shown while resume must remain disabled.
    writeSegment(directory / "00003.ts");
    source.source.segmentPaths.push_back(
        (directory / "00003.ts").string());
    writeIndex(directory / "index", 1, 3);
    auto unmapped = projectDeviceRecordingResume(recording, source,
                                                "hls-fmp4", false);
    assert(unmapped.durationSeconds == 6);
    assert(!unmapped.supported);

    // Unexpected in-progress marker and missing index must fail closed.
    {
        std::ofstream timer(directory / ".timer");
        timer << "in progress";
    }
    assert(!projectDeviceRecordingResume(recording, source,
                                          "hls-fmp4", false).supported);
    std::filesystem::remove(directory / ".timer");
    std::filesystem::remove(directory / "index");
    auto unindexed = projectDeviceRecordingResume(recording, source,
                                                  "hls-fmp4", false);
    assert(unindexed.durationSeconds == 0);
    assert(!unindexed.supported);

    std::filesystem::remove_all(directory, ignored);
    return 0;
}

#include "MediaHlsArtifactReader.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

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
    const auto root = std::filesystem::temp_directory_path() /
        "vdr-suite-media-hls-artifact-reader-test";
    const auto workspace = root / "ms_test123";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(workspace);

    writeFile(workspace / "master.m3u8", "#EXTM3U\n");
    writeFile(workspace / "init.mp4", "init");
    writeFile(workspace / "segment-000001.m4s", "segment-one");
    writeFile(workspace / "segment-000002.ts", "segment-two");
    writeFile(workspace / "ffmpeg.log", "private-log");

    MediaHlsArtifactReader reader(root.string());

    {
        const auto result = reader.read("ms_test123", "master.m3u8");
        assert(result.found);
        assert(result.contentType == "application/vnd.apple.mpegurl");
        assert(asString(result.bytes) == "#EXTM3U\n");
    }

    {
        const auto result = reader.read("ms_test123", "init.mp4");
        assert(result.found);
        assert(result.contentType == "video/mp4");
    }

    {
        const auto result = reader.read("ms_test123", "segment-000001.m4s");
        assert(result.found);
        assert(result.contentType == "video/iso.segment");
        assert(asString(result.bytes) == "segment-one");
    }

    {
        const auto result = reader.read("ms_test123", "segment-000002.ts");
        assert(result.found);
        assert(result.contentType == "video/mp2t");
    }

    {
        const auto result = reader.read("ms_test123", "ffmpeg.log");
        assert(!result.found);
        assert(result.reasonCode == "invalid_media_artifact_request");
    }

    {
        const auto result = reader.read("../escape", "master.m3u8");
        assert(!result.found);
        assert(result.reasonCode == "invalid_media_artifact_request");
    }

    {
        const auto result = reader.read("ms_test123", "segment-1.m4s");
        assert(!result.found);
        assert(result.reasonCode == "invalid_media_artifact_request");
    }

    {
        const auto missing = reader.read("ms_test123", "segment-999999.m4s");
        assert(!missing.found);
        assert(missing.reasonCode == "media_artifact_not_ready");
    }

    const auto outside = root / "outside.m4s";
    writeFile(outside, "outside");
    std::filesystem::create_symlink(outside, workspace / "segment-000003.m4s");
    {
        const auto result = reader.read("ms_test123", "segment-000003.m4s");
        assert(!result.found);
        assert(result.reasonCode == "media_artifact_not_regular");
    }

    std::filesystem::remove_all(root);
    return 0;
}
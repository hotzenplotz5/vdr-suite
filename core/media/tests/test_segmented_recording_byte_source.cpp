#include "SegmentedRecordingByteSource.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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
        "vdr-suite-segmented-recording-byte-source-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto first = root / "00001.ts";
    const auto second = root / "00002.ts";
    const auto third = root / "00003.ts";
    writeFile(first, "ABCDE");
    writeFile(second, "FGHIJ");

    std::vector<std::string> segments = {
        first.string(),
        second.string()
    };

    SegmentedRecordingByteSource source(
        [&segments]() { return segments; },
        true,
        6);

    {
        const RecordingByteExtent extent = source.refreshExtent();
        assert(extent.readableBytes == 10);
        assert(extent.segmentCount == 2);
        assert(extent.growing);
    }

    {
        const auto result = source.read(3, 6);
        assert(result.success);
        assert(asString(result.bytes) == "DEFGHI");
        assert(result.nextOffset == 9);
        assert(!result.endOfReadableExtent);
    }

    {
        // The byte source enforces its own maximum read size even when a
        // future Gateway or client asks for a larger range.
        const auto result = source.read(0, 1000);
        assert(result.success);
        assert(result.bytes.size() == 6);
        assert(asString(result.bytes) == "ABCDEF");
    }

    {
        const auto result = source.read(10, 16);
        assert(result.success);
        assert(result.bytes.empty());
        assert(result.endOfReadableExtent);
    }

    {
        const auto result = source.read(11, 1);
        assert(!result.success);
        assert(result.reasonCode == "recording_range_not_satisfiable");
    }

    // Growing recordings can add a new VDR segment without replacing the
    // logical source object. The next extent/read observes the new catalog.
    writeFile(third, "KLMN");
    segments.push_back(third.string());

    {
        const RecordingByteExtent extent = source.refreshExtent();
        assert(extent.readableBytes == 14);
        assert(extent.segmentCount == 3);
    }

    {
        const auto result = source.read(9, 5);
        assert(result.success);
        assert(asString(result.bytes) == "JKLMN");
        assert(result.endOfReadableExtent);
    }

    std::filesystem::remove_all(root);
    return 0;
}
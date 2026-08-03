#include "FilesystemSeriesArtworkFallbackMaterializer.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{
std::uint32_t crc32(const std::vector<unsigned char>& bytes)
{
    std::uint32_t crc = 0xffffffffU;
    for (const unsigned char byte : bytes)
    {
        crc ^= static_cast<std::uint32_t>(byte);
        for (int bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1U) ^
                (0xedb88320U &
                 static_cast<std::uint32_t>(-
                     static_cast<std::int32_t>(crc & 1U)));
        }
    }
    return crc ^ 0xffffffffU;
}

void append32(std::vector<unsigned char>& bytes, std::uint32_t value)
{
    bytes.push_back(static_cast<unsigned char>((value >> 24U) & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<unsigned char>(value & 0xffU));
}

void appendChunk(
    std::vector<unsigned char>& bytes,
    const std::string& type,
    const std::vector<unsigned char>& data)
{
    append32(bytes, static_cast<std::uint32_t>(data.size()));
    std::vector<unsigned char> crcInput(type.begin(), type.end());
    crcInput.insert(crcInput.end(), data.begin(), data.end());
    bytes.insert(bytes.end(), type.begin(), type.end());
    bytes.insert(bytes.end(), data.begin(), data.end());
    append32(bytes, crc32(crcInput));
}

std::vector<unsigned char> png(int width, int height)
{
    std::vector<unsigned char> bytes = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
    };
    std::vector<unsigned char> header;
    append32(header, static_cast<std::uint32_t>(width));
    append32(header, static_cast<std::uint32_t>(height));
    header.insert(header.end(), {0x08, 0x02, 0x00, 0x00, 0x00});
    appendChunk(bytes, "IHDR", header);
    appendChunk(
        bytes,
        "IDAT",
        {0x78, 0x9c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01});
    appendChunk(bytes, "IEND", {});
    return bytes;
}

void writeBytes(
    const std::filesystem::path& path,
    const std::vector<unsigned char>& bytes)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    assert(file);
    file.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    assert(file.good());
}

std::vector<unsigned char> readBytes(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    assert(file);
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

SeriesArtworkFallbackMaterializationRequest requestFor(
    const std::filesystem::path& source,
    int width = 640,
    int height = 360)
{
    SeriesArtworkFallbackMaterializationRequest request;
    request.backendId = "backend one";
    request.channelId = "S19.2E-1-1011-11100";
    request.eventId = "12345";
    request.candidate.available = true;
    request.candidate.provider = "example-provider";
    request.candidate.origin = EpgScraperArtworkOrigin::ExternalFallback;
    request.candidate.path = source.string();
    request.candidate.width = width;
    request.candidate.height = height;
    return request;
}
}

int main()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-materializer-" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);

    const std::filesystem::path incoming = root / "incoming";
    const std::filesystem::path cache = root / "cache";
    FilesystemSeriesArtworkFallbackMaterializerConfig config;
    config.allowedSourceRoots = {incoming.string()};
    config.cacheRoot = cache.string();
    config.maximumSourceBytes = 1024U;
    config.maximumDimension = 4096;
    config.maximumPixels = 4000000U;

    FilesystemSeriesArtworkFallbackMaterializer materializer(config);

    const std::filesystem::path validSource = incoming / "spoofed.txt";
    const std::vector<unsigned char> validPng = png(640, 360);
    writeBytes(validSource, validPng);

    const auto stored = materializer.materialize(requestFor(validSource));
    assert(stored.valid());
    assert(stored.artwork.provider == "example-provider");
    assert(stored.artwork.origin == EpgScraperArtworkOrigin::ExternalFallback);
    assert(stored.artwork.width == 640);
    assert(stored.artwork.height == 360);
    assert(std::filesystem::path(stored.artwork.path).extension() == ".png");
    assert(std::filesystem::exists(stored.artwork.path));
    assert(readBytes(stored.artwork.path) == validPng);

    const std::filesystem::path outside = root / "outside.png";
    writeBytes(outside, validPng);
    const auto outsideResult = materializer.materialize(requestFor(outside));
    assert(outsideResult.attempted);
    assert(!outsideResult.stored);

    const std::filesystem::path symlink = incoming / "link.png";
    std::filesystem::create_symlink(outside, symlink);
    const auto symlinkResult = materializer.materialize(requestFor(symlink));
    assert(!symlinkResult.stored);

    const auto mismatched = materializer.materialize(
        requestFor(validSource, 1280, 720));
    assert(!mismatched.stored);

    const std::filesystem::path unsupported = incoming / "not-image.jpg";
    writeBytes(unsupported, {'n', 'o', 't', '-', 'i', 'm', 'a', 'g', 'e'});
    const auto unsupportedResult = materializer.materialize(
        requestFor(unsupported, 1, 1));
    assert(!unsupportedResult.stored);

    std::vector<unsigned char> corruptPng = validPng;
    corruptPng[corruptPng.size() - 1U] ^= 0xffU;
    const std::filesystem::path corrupt = incoming / "corrupt.png";
    writeBytes(corrupt, corruptPng);
    const auto corruptResult = materializer.materialize(requestFor(corrupt));
    assert(!corruptResult.stored);

    FilesystemSeriesArtworkFallbackMaterializerConfig tinyConfig = config;
    tinyConfig.maximumSourceBytes = 16U;
    FilesystemSeriesArtworkFallbackMaterializer tinyMaterializer(tinyConfig);
    const auto oversized = tinyMaterializer.materialize(requestFor(validSource));
    assert(!oversized.stored);

    FilesystemSeriesArtworkFallbackMaterializerConfig dimensionConfig = config;
    dimensionConfig.maximumDimension = 320;
    FilesystemSeriesArtworkFallbackMaterializer dimensionMaterializer(
        dimensionConfig);
    const auto tooWide = dimensionMaterializer.materialize(
        requestFor(validSource));
    assert(!tooWide.stored);

    const std::filesystem::path hostileCache = root / "hostile-cache";
    std::filesystem::create_directories(hostileCache);
    const std::filesystem::path escaped = root / "escaped";
    std::filesystem::create_directories(escaped);
    const std::string backendHex =
        "6261636b656e64206f6e65";
    std::filesystem::create_symlink(escaped, hostileCache / backendHex);
    FilesystemSeriesArtworkFallbackMaterializerConfig hostileConfig = config;
    hostileConfig.cacheRoot = hostileCache.string();
    FilesystemSeriesArtworkFallbackMaterializer hostileMaterializer(
        hostileConfig);
    const auto hostile = hostileMaterializer.materialize(requestFor(validSource));
    assert(!hostile.stored);
    assert(std::filesystem::is_empty(escaped));

    std::filesystem::remove_all(root);
    return 0;
}

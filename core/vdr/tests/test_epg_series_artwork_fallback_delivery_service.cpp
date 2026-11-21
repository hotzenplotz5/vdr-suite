#include "Database.h"
#include "EpgSeriesArtworkFallbackDeliveryService.h"
#include "EpgSeriesArtworkFallbackRepository.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

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

std::vector<unsigned char> jpeg(int width, int height)
{
    return {
        0xff, 0xd8,
        0xff, 0xc0, 0x00, 0x07, 0x08,
        static_cast<unsigned char>((height >> 8) & 0xff),
        static_cast<unsigned char>(height & 0xff),
        static_cast<unsigned char>((width >> 8) & 0xff),
        static_cast<unsigned char>(width & 0xff),
        0xff, 0xd9
    };
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

EpgArtworkReference referenceFor(
    const std::string& eventId,
    const std::filesystem::path& path,
    int width = 640,
    int height = 360)
{
    EpgArtworkReference reference;
    reference.backendId = "backend";
    reference.channelId = "channel";
    reference.eventId = eventId;
    reference.provider = "tmdb";
    reference.origin = EpgArtworkReferenceOrigin::ExternalFallback;
    reference.path = path.string();
    reference.width = width;
    reference.height = height;
    reference.resolvedAt = 1234;
    return reference;
}
}

int main()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-fallback-delivery-" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);

    const std::filesystem::path cache = root / "cache";
    const std::filesystem::path outside = root / "outside";
    std::filesystem::create_directories(cache);
    std::filesystem::create_directories(outside);

    Database database;
    assert(database.open(":memory:"));
    EpgSeriesArtworkFallbackRepository repository(database);
    assert(repository.ensureSchema());

    EpgSeriesArtworkFallbackDeliveryConfig config;
    config.managedRoots = {cache.string()};
    config.maximumBytes = 1024U;
    config.maximumDimension = 4096;
    config.maximumPixels = 4000000U;
    EpgSeriesArtworkFallbackDeliveryService delivery(repository, config);

    const std::vector<unsigned char> validPng = png(640, 360);
    const std::filesystem::path pngPath = cache / "series.png";
    writeBytes(pngPath, validPng);
    assert(repository.upsert(referenceFor("png", pngPath)));

    const EpgSeriesArtworkFallbackAsset pngAsset =
        delivery.loadSeriesArtworkFallback("backend", "channel", "png");
    assert(pngAsset.valid());
    assert(pngAsset.contentType == "image/png");
    assert(pngAsset.width == 640);
    assert(pngAsset.height == 360);
    assert(pngAsset.content == std::string(
        reinterpret_cast<const char*>(validPng.data()),
        validPng.size()));

    const std::vector<unsigned char> validJpeg = jpeg(1280, 720);
    const std::filesystem::path jpegPath = cache / "series.jpg";
    writeBytes(jpegPath, validJpeg);
    assert(repository.upsert(referenceFor("jpeg", jpegPath, 1280, 720)));
    const EpgSeriesArtworkFallbackAsset jpegAsset =
        delivery.loadSeriesArtworkFallback("backend", "channel", "jpeg");
    assert(jpegAsset.valid());
    assert(jpegAsset.contentType == "image/jpeg");
    assert(jpegAsset.width == 1280);
    assert(jpegAsset.height == 720);

    const std::filesystem::path outsidePath = outside / "outside.png";
    writeBytes(outsidePath, validPng);
    assert(repository.upsert(referenceFor("outside", outsidePath)));
    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "channel", "outside").valid());

    const std::filesystem::path symlinkPath = cache / "link.png";
    std::filesystem::create_symlink(outsidePath, symlinkPath);
    assert(repository.upsert(referenceFor("symlink", symlinkPath)));
    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "channel", "symlink").valid());

    const std::filesystem::path outsideDirectory = outside / "directory";
    const std::filesystem::path outsideNested = outsideDirectory / "nested.png";
    writeBytes(outsideNested, validPng);
    const std::filesystem::path linkedDirectory = cache / "linked-directory";
    std::filesystem::create_directory_symlink(
        outsideDirectory,
        linkedDirectory);
    assert(repository.upsert(referenceFor(
        "directory-symlink",
        linkedDirectory / "nested.png")));
    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "channel", "directory-symlink").valid());

    const std::filesystem::path corruptPath = cache / "corrupt.png";
    std::vector<unsigned char> corruptPng = validPng;
    corruptPng.back() ^= 0xffU;
    writeBytes(corruptPath, corruptPng);
    assert(repository.upsert(referenceFor("corrupt", corruptPath)));
    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "channel", "corrupt").valid());

    const std::filesystem::path spoofedPath = cache / "spoofed.png";
    writeBytes(spoofedPath, validJpeg);
    assert(repository.upsert(referenceFor(
        "spoofed",
        spoofedPath,
        1280,
        720)));
    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "channel", "spoofed").valid());

    assert(repository.upsert(referenceFor(
        "dimension-mismatch",
        pngPath,
        1280,
        720)));
    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "channel", "dimension-mismatch").valid());

    const std::filesystem::path traversalPath =
        cache / "subdirectory" / ".." / "series.png";
    std::filesystem::create_directories(cache / "subdirectory");
    assert(!repository.upsert(referenceFor("traversal", traversalPath)));
    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "channel", "traversal").valid());

    const std::filesystem::path rootSymlink = root / "cache-link";
    std::filesystem::create_directory_symlink(cache, rootSymlink);
    EpgSeriesArtworkFallbackDeliveryConfig symlinkRootConfig = config;
    symlinkRootConfig.managedRoots = {rootSymlink.string()};
    EpgSeriesArtworkFallbackDeliveryService symlinkRootDelivery(
        repository,
        symlinkRootConfig);
    assert(!symlinkRootDelivery.loadSeriesArtworkFallback(
        "backend", "channel", "png").valid());

    EpgSeriesArtworkFallbackDeliveryConfig tinyConfig = config;
    tinyConfig.maximumBytes = 16U;
    EpgSeriesArtworkFallbackDeliveryService tinyDelivery(
        repository,
        tinyConfig);
    assert(!tinyDelivery.loadSeriesArtworkFallback(
        "backend", "channel", "png").valid());

    EpgSeriesArtworkFallbackDeliveryConfig dimensionConfig = config;
    dimensionConfig.maximumDimension = 320;
    EpgSeriesArtworkFallbackDeliveryService dimensionDelivery(
        repository,
        dimensionConfig);
    assert(!dimensionDelivery.loadSeriesArtworkFallback(
        "backend", "channel", "png").valid());

    EpgSeriesArtworkFallbackDeliveryConfig pixelConfig = config;
    pixelConfig.maximumPixels = 100U;
    EpgSeriesArtworkFallbackDeliveryService pixelDelivery(
        repository,
        pixelConfig);
    assert(!pixelDelivery.loadSeriesArtworkFallback(
        "backend", "channel", "png").valid());

    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "channel", "missing").valid());
    assert(!delivery.loadSeriesArtworkFallback(
        "backend", "", "png").valid());

    std::filesystem::remove_all(root);
    return 0;
}

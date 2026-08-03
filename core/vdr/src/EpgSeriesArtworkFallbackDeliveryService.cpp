#include "EpgSeriesArtworkFallbackDeliveryService.h"

#include "EpgArtworkReference.h"
#include "EpgSeriesArtworkFallbackRepository.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fcntl.h>
#include <limits>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
class FileDescriptor
{
public:
    explicit FileDescriptor(int value = -1)
        : value_(value)
    {
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept
        : value_(other.release())
    {
    }

    FileDescriptor& operator=(FileDescriptor&& other) noexcept
    {
        if (this != &other)
        {
            reset(other.release());
        }
        return *this;
    }

    ~FileDescriptor()
    {
        reset();
    }

    int get() const
    {
        return value_;
    }

    bool valid() const
    {
        return value_ >= 0;
    }

    int release()
    {
        const int value = value_;
        value_ = -1;
        return value;
    }

    void reset(int value = -1)
    {
        if (value_ >= 0)
        {
            ::close(value_);
        }
        value_ = value;
    }

private:
    int value_;
};

struct ImageDescription
{
    std::string contentType;
    int width = 0;
    int height = 0;

    bool valid() const
    {
        return (contentType == "image/png" || contentType == "image/jpeg") &&
            width > 0 && height > 0;
    }
};

std::uint32_t bigEndian32(const unsigned char* value)
{
    return
        (static_cast<std::uint32_t>(value[0]) << 24U) |
        (static_cast<std::uint32_t>(value[1]) << 16U) |
        (static_cast<std::uint32_t>(value[2]) << 8U) |
        static_cast<std::uint32_t>(value[3]);
}

std::uint16_t bigEndian16(const unsigned char* value)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(value[0]) << 8U) |
        static_cast<std::uint16_t>(value[1]));
}

std::uint32_t pngCrc32(
    const unsigned char* data,
    std::size_t size)
{
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t index = 0; index < size; ++index)
    {
        crc ^= static_cast<std::uint32_t>(data[index]);
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

ImageDescription describePng(const std::vector<unsigned char>& bytes)
{
    static constexpr unsigned char Signature[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
    };

    if (bytes.size() < sizeof(Signature) ||
        !std::equal(
            std::begin(Signature),
            std::end(Signature),
            bytes.begin()))
    {
        return {};
    }

    ImageDescription description;
    bool sawHeader = false;
    bool sawImageData = false;
    std::size_t offset = sizeof(Signature);

    while (offset + 12U <= bytes.size())
    {
        const std::uint32_t length = bigEndian32(bytes.data() + offset);
        const std::size_t chunkSize =
            12U + static_cast<std::size_t>(length);
        if (chunkSize < 12U || offset + chunkSize > bytes.size())
        {
            return {};
        }

        const unsigned char* type = bytes.data() + offset + 4U;
        const unsigned char* data = bytes.data() + offset + 8U;
        const std::uint32_t expectedCrc =
            bigEndian32(bytes.data() + offset + 8U + length);
        if (pngCrc32(type, 4U + length) != expectedCrc)
        {
            return {};
        }

        const bool isHeader =
            type[0] == 'I' && type[1] == 'H' &&
            type[2] == 'D' && type[3] == 'R';
        const bool isImageData =
            type[0] == 'I' && type[1] == 'D' &&
            type[2] == 'A' && type[3] == 'T';
        const bool isEnd =
            type[0] == 'I' && type[1] == 'E' &&
            type[2] == 'N' && type[3] == 'D';

        if (!sawHeader)
        {
            if (!isHeader || length != 13U)
            {
                return {};
            }

            const std::uint32_t width = bigEndian32(data);
            const std::uint32_t height = bigEndian32(data + 4U);
            if (width == 0U || height == 0U ||
                width > static_cast<std::uint32_t>(
                    std::numeric_limits<int>::max()) ||
                height > static_cast<std::uint32_t>(
                    std::numeric_limits<int>::max()))
            {
                return {};
            }

            description.contentType = "image/png";
            description.width = static_cast<int>(width);
            description.height = static_cast<int>(height);
            sawHeader = true;
        }
        else if (isHeader)
        {
            return {};
        }

        if (isImageData)
        {
            sawImageData = true;
        }

        if (isEnd)
        {
            if (length != 0U || !sawImageData ||
                offset + chunkSize != bytes.size())
            {
                return {};
            }
            return description;
        }

        offset += chunkSize;
    }

    return {};
}

bool isStartOfFrameMarker(unsigned char marker)
{
    switch (marker)
    {
    case 0xc0:
    case 0xc1:
    case 0xc2:
    case 0xc3:
    case 0xc5:
    case 0xc6:
    case 0xc7:
    case 0xc9:
    case 0xca:
    case 0xcb:
    case 0xcd:
    case 0xce:
    case 0xcf:
        return true;
    default:
        return false;
    }
}

ImageDescription describeJpeg(const std::vector<unsigned char>& bytes)
{
    if (bytes.size() < 4U ||
        bytes[0] != 0xffU || bytes[1] != 0xd8U ||
        bytes[bytes.size() - 2U] != 0xffU ||
        bytes[bytes.size() - 1U] != 0xd9U)
    {
        return {};
    }

    std::size_t offset = 2U;
    while (offset < bytes.size())
    {
        while (offset < bytes.size() && bytes[offset] == 0xffU)
        {
            ++offset;
        }
        if (offset >= bytes.size())
        {
            break;
        }

        const unsigned char marker = bytes[offset++];
        if (marker == 0x00U || marker == 0xd8U)
        {
            continue;
        }
        if (marker == 0xd9U || marker == 0xdaU)
        {
            break;
        }
        if (marker == 0x01U || (marker >= 0xd0U && marker <= 0xd7U))
        {
            continue;
        }
        if (offset + 2U > bytes.size())
        {
            return {};
        }

        const std::uint16_t segmentLength =
            bigEndian16(bytes.data() + offset);
        if (segmentLength < 2U ||
            offset + static_cast<std::size_t>(segmentLength) > bytes.size())
        {
            return {};
        }

        if (isStartOfFrameMarker(marker))
        {
            if (segmentLength < 7U)
            {
                return {};
            }

            const std::uint16_t height =
                bigEndian16(bytes.data() + offset + 3U);
            const std::uint16_t width =
                bigEndian16(bytes.data() + offset + 5U);
            if (width == 0U || height == 0U)
            {
                return {};
            }

            ImageDescription description;
            description.contentType = "image/jpeg";
            description.width = static_cast<int>(width);
            description.height = static_cast<int>(height);
            return description;
        }

        offset += static_cast<std::size_t>(segmentLength);
    }

    return {};
}

ImageDescription describeImage(const std::vector<unsigned char>& bytes)
{
    ImageDescription description = describePng(bytes);
    return description.valid() ? description : describeJpeg(bytes);
}

std::string lowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

bool extensionMatches(
    const std::filesystem::path& path,
    const std::string& contentType)
{
    const std::string extension = lowerAscii(path.extension().string());
    if (contentType == "image/png")
    {
        return extension == ".png";
    }
    if (contentType == "image/jpeg")
    {
        return extension == ".jpg" || extension == ".jpeg";
    }
    return false;
}

bool validProviderName(const std::string& provider)
{
    if (provider.empty() || provider.size() > 64U ||
        provider == "none" || provider == "tvscraper")
    {
        return false;
    }

    return std::all_of(
        provider.begin(),
        provider.end(),
        [](unsigned char character)
        {
            return std::islower(character) || std::isdigit(character) ||
                character == '-' || character == '_' || character == '.';
        });
}

bool strictDescendant(
    const std::filesystem::path& candidate,
    const std::filesystem::path& root)
{
    if (candidate == root)
    {
        return false;
    }

    auto candidatePart = candidate.begin();
    for (auto rootPart = root.begin(); rootPart != root.end();
         ++rootPart, ++candidatePart)
    {
        if (candidatePart == candidate.end() || *candidatePart != *rootPart)
        {
            return false;
        }
    }
    return true;
}

bool safeComponent(const std::filesystem::path& component)
{
    const std::string value = component.string();
    return !value.empty() && value != "." && value != ".." &&
        value.find('/') == std::string::npos;
}

FileDescriptor openAbsoluteDirectoryNoFollow(
    const std::filesystem::path& directory)
{
    if (!directory.is_absolute())
    {
        return {};
    }

    FileDescriptor current(::open(
        "/",
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!current.valid())
    {
        return {};
    }

    for (const auto& component : directory.relative_path())
    {
        if (!safeComponent(component))
        {
            return {};
        }

        FileDescriptor child(::openat(
            current.get(),
            component.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
        if (!child.valid())
        {
            return {};
        }
        current = std::move(child);
    }

    return current;
}

bool openManagedFile(
    const std::filesystem::path& candidate,
    const std::vector<std::string>& configuredRoots,
    FileDescriptor& file)
{
    file.reset();
    if (!candidate.is_absolute())
    {
        return false;
    }

    const std::filesystem::path normalizedCandidate =
        candidate.lexically_normal();

    for (const std::string& configuredRoot : configuredRoots)
    {
        const std::filesystem::path normalizedRoot =
            std::filesystem::path(configuredRoot).lexically_normal();
        if (!normalizedRoot.is_absolute() || normalizedRoot == "/" ||
            !strictDescendant(normalizedCandidate, normalizedRoot))
        {
            continue;
        }

        const std::filesystem::path relative =
            normalizedCandidate.lexically_relative(normalizedRoot);
        if (relative.empty() || relative.is_absolute())
        {
            continue;
        }

        FileDescriptor directory = openAbsoluteDirectoryNoFollow(normalizedRoot);
        if (!directory.valid())
        {
            continue;
        }

        std::vector<std::filesystem::path> components(
            relative.begin(),
            relative.end());
        if (components.empty())
        {
            continue;
        }

        bool valid = true;
        for (std::size_t index = 0; index + 1U < components.size(); ++index)
        {
            if (!safeComponent(components[index]))
            {
                valid = false;
                break;
            }

            FileDescriptor child(::openat(
                directory.get(),
                components[index].c_str(),
                O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
            if (!child.valid())
            {
                valid = false;
                break;
            }
            directory = std::move(child);
        }
        if (!valid || !safeComponent(components.back()))
        {
            continue;
        }

        FileDescriptor candidateFile(::openat(
            directory.get(),
            components.back().c_str(),
            O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
        if (candidateFile.valid())
        {
            file = std::move(candidateFile);
            return true;
        }
    }

    return false;
}

bool readManagedFile(
    const std::filesystem::path& path,
    const std::vector<std::string>& managedRoots,
    std::uintmax_t maximumBytes,
    std::vector<unsigned char>& bytes)
{
    bytes.clear();
    if (maximumBytes == 0U ||
        maximumBytes > static_cast<std::uintmax_t>(
            std::numeric_limits<std::size_t>::max()))
    {
        return false;
    }

    FileDescriptor file;
    if (!openManagedFile(path, managedRoots, file))
    {
        return false;
    }

    struct stat before {};
    if (::fstat(file.get(), &before) != 0 ||
        !S_ISREG(before.st_mode) || before.st_size <= 0 ||
        static_cast<std::uintmax_t>(before.st_size) > maximumBytes)
    {
        return false;
    }

    bytes.resize(static_cast<std::size_t>(before.st_size));
    std::size_t offset = 0U;
    while (offset < bytes.size())
    {
        const ssize_t count = ::read(
            file.get(),
            bytes.data() + offset,
            bytes.size() - offset);
        if (count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        if (count == 0)
        {
            return false;
        }
        offset += static_cast<std::size_t>(count);
    }

    unsigned char extra = 0U;
    ssize_t extraCount = 0;
    do
    {
        extraCount = ::read(file.get(), &extra, 1U);
    }
    while (extraCount < 0 && errno == EINTR);
    if (extraCount != 0)
    {
        return false;
    }

    struct stat after {};
    return ::fstat(file.get(), &after) == 0 &&
        S_ISREG(after.st_mode) &&
        before.st_dev == after.st_dev &&
        before.st_ino == after.st_ino &&
        before.st_size == after.st_size;
}

bool acceptableImage(
    const ImageDescription& description,
    const EpgArtworkReference& reference,
    const EpgSeriesArtworkFallbackDeliveryConfig& config)
{
    if (!description.valid() ||
        description.width != reference.width ||
        description.height != reference.height ||
        config.maximumDimension <= 0 ||
        description.width > config.maximumDimension ||
        description.height > config.maximumDimension)
    {
        return false;
    }

    const std::uint64_t pixels =
        static_cast<std::uint64_t>(description.width) *
        static_cast<std::uint64_t>(description.height);
    return pixels > 0U && pixels <= config.maximumPixels;
}
}

EpgSeriesArtworkFallbackDeliveryService::
EpgSeriesArtworkFallbackDeliveryService(
    EpgSeriesArtworkFallbackRepository& repository,
    EpgSeriesArtworkFallbackDeliveryConfig config)
    : repository_(repository),
      config_(std::move(config))
{
}

EpgSeriesArtworkFallbackAsset
EpgSeriesArtworkFallbackDeliveryService::loadSeriesArtworkFallback(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    EpgSeriesArtworkFallbackAsset asset;
    if (channelId.empty() || eventId.empty() || config_.managedRoots.empty())
    {
        return asset;
    }

    const EpgArtworkReference reference = repository_.find(
        backendId.empty() ? "default" : backendId,
        channelId,
        eventId);
    if (!reference.valid() ||
        reference.origin != EpgArtworkReferenceOrigin::ExternalFallback ||
        !validProviderName(reference.provider) ||
        reference.resolvedAt <= 0 ||
        !std::filesystem::path(reference.path).is_absolute())
    {
        return asset;
    }

    std::vector<unsigned char> bytes;
    if (!readManagedFile(
            reference.path,
            config_.managedRoots,
            config_.maximumBytes,
            bytes))
    {
        return asset;
    }

    const ImageDescription description = describeImage(bytes);
    if (!acceptableImage(description, reference, config_) ||
        !extensionMatches(reference.path, description.contentType))
    {
        return asset;
    }

    asset.contentType = description.contentType;
    asset.content.assign(
        reinterpret_cast<const char*>(bytes.data()),
        bytes.size());
    asset.width = description.width;
    asset.height = description.height;
    return asset;
}

#include "FilesystemSeriesArtworkFallbackMaterializer.h"

#include "EpgArtworkPathPolicy.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <fcntl.h>
#include <iterator>
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
    std::string extension;
    int width = 0;
    int height = 0;

    bool valid() const
    {
        return !extension.empty() && width > 0 && height > 0;
    }
};

std::atomic<unsigned long long> TemporarySequence{0};

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

    ImageDescription description;
    if (bytes.size() < 8U ||
        !std::equal(
            std::begin(Signature),
            std::end(Signature),
            bytes.begin()))
    {
        return description;
    }

    bool sawHeader = false;
    bool sawImageData = false;
    std::size_t offset = 8U;
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
            if (width == 0U ||
                height == 0U ||
                width > static_cast<std::uint32_t>(
                    std::numeric_limits<int>::max()) ||
                height > static_cast<std::uint32_t>(
                    std::numeric_limits<int>::max()))
            {
                return {};
            }
            description.extension = ".png";
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
            if (length != 0U ||
                !sawImageData ||
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

ImageDescription describeJpeg(const std::vector<unsigned char>& bytes)
{
    ImageDescription description;
    if (bytes.size() < 4U ||
        bytes[0] != 0xffU ||
        bytes[1] != 0xd8U ||
        bytes[bytes.size() - 2U] != 0xffU ||
        bytes[bytes.size() - 1U] != 0xd9U)
    {
        return description;
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

            description.extension = ".jpg";
            description.width = static_cast<int>(width);
            description.height = static_cast<int>(height);
            return description;
        }

        offset += static_cast<std::size_t>(segmentLength);
    }

    return description;
}

ImageDescription describeImage(const std::vector<unsigned char>& bytes)
{
    ImageDescription description = describePng(bytes);
    if (description.valid())
    {
        return description;
    }
    return describeJpeg(bytes);
}

bool acceptableDimensions(
    const ImageDescription& description,
    const FilesystemSeriesArtworkFallbackMaterializerConfig& config)
{
    if (!description.valid() ||
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

bool isPathWithinRoot(
    const std::filesystem::path& path,
    const std::filesystem::path& root)
{
    auto pathIterator = path.begin();
    auto rootIterator = root.begin();

    for (; rootIterator != root.end(); ++rootIterator, ++pathIterator)
    {
        if (pathIterator == path.end() || *pathIterator != *rootIterator)
        {
            return false;
        }
    }

    return true;
}

bool openSourceBelowAllowedRoot(
    const std::string& sourcePath,
    const std::vector<std::string>& allowedRoots,
    FileDescriptor& source)
{
    source.reset();
    std::error_code error;
    const std::filesystem::path canonicalSource =
        std::filesystem::weakly_canonical(sourcePath, error);
    if (error || canonicalSource.empty() || !canonicalSource.is_absolute())
    {
        return false;
    }

    for (const std::string& configuredRoot : allowedRoots)
    {
        error.clear();
        const std::filesystem::path canonicalRoot =
            std::filesystem::weakly_canonical(configuredRoot, error);
        if (error ||
            canonicalRoot.empty() ||
            !canonicalRoot.is_absolute() ||
            canonicalSource == canonicalRoot ||
            !isPathWithinRoot(canonicalSource, canonicalRoot))
        {
            continue;
        }

        const std::filesystem::path relative =
            canonicalSource.lexically_relative(canonicalRoot);
        if (relative.empty() || relative.is_absolute())
        {
            continue;
        }

        FileDescriptor directory(::open(
            canonicalRoot.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
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
            const std::string component = components[index].string();
            if (component.empty() || component == "." || component == "..")
            {
                valid = false;
                break;
            }

            FileDescriptor child(::openat(
                directory.get(),
                component.c_str(),
                O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
            if (!child.valid())
            {
                valid = false;
                break;
            }
            directory = std::move(child);
        }
        if (!valid)
        {
            continue;
        }

        const std::string filename = components.back().string();
        if (filename.empty() || filename == "." || filename == "..")
        {
            continue;
        }

        FileDescriptor candidate(::openat(
            directory.get(),
            filename.c_str(),
            O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
        if (candidate.valid())
        {
            source = std::move(candidate);
            return true;
        }
    }

    return false;
}

bool readSource(
    const std::string& path,
    const std::vector<std::string>& allowedRoots,
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
    if (!openSourceBelowAllowedRoot(path, allowedRoots, file))
    {
        return false;
    }

    struct stat status {};
    if (::fstat(file.get(), &status) != 0 ||
        !S_ISREG(status.st_mode) ||
        status.st_size <= 0 ||
        static_cast<std::uintmax_t>(status.st_size) > maximumBytes)
    {
        return false;
    }

    bytes.resize(static_cast<std::size_t>(status.st_size));
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
    while (true)
    {
        const ssize_t count = ::read(file.get(), &extra, 1U);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count != 0)
        {
            return false;
        }
        break;
    }

    return true;
}

std::string hexComponent(const std::string& value)
{
    static constexpr char Digits[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(value.size() * 2U);
    for (const unsigned char byte : value)
    {
        encoded.push_back(Digits[(byte >> 4U) & 0x0fU]);
        encoded.push_back(Digits[byte & 0x0fU]);
    }
    return encoded;
}

FileDescriptor openOrCreateDirectoryAt(int parent, const std::string& name)
{
    if (name.empty())
    {
        return FileDescriptor();
    }

    if (::mkdirat(parent, name.c_str(), 0750) != 0 && errno != EEXIST)
    {
        return FileDescriptor();
    }

    FileDescriptor directory(::openat(
        parent,
        name.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!directory.valid() || ::fchmod(directory.get(), 0750) != 0)
    {
        return FileDescriptor();
    }
    return directory;
}

bool writeAll(int file, const std::vector<unsigned char>& bytes)
{
    std::size_t offset = 0U;
    while (offset < bytes.size())
    {
        const ssize_t count = ::write(
            file,
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
    return true;
}

bool atomicallyStore(
    const std::string& cacheRoot,
    const SeriesArtworkFallbackMaterializationRequest& request,
    const ImageDescription& description,
    const std::vector<unsigned char>& bytes,
    std::string& destinationPath)
{
    destinationPath.clear();
    const std::filesystem::path configuredRoot =
        std::filesystem::path(cacheRoot).lexically_normal();
    if (!configuredRoot.is_absolute() ||
        configuredRoot == configuredRoot.root_path())
    {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(configuredRoot, error);
    if (error)
    {
        return false;
    }

    FileDescriptor directory(::open(
        configuredRoot.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!directory.valid() || ::fchmod(directory.get(), 0750) != 0)
    {
        return false;
    }

    const std::string backendComponent = hexComponent(request.backendId);
    const std::string channelComponent = hexComponent(request.channelId);
    const std::string eventComponent = hexComponent(request.eventId);
    if (backendComponent.size() > 256U ||
        channelComponent.size() > 512U ||
        eventComponent.size() > 512U)
    {
        return false;
    }

    for (const std::string* component : {
             &backendComponent,
             &channelComponent,
             &eventComponent})
    {
        FileDescriptor child = openOrCreateDirectoryAt(
            directory.get(),
            *component);
        if (!child.valid())
        {
            return false;
        }
        directory = std::move(child);
    }

    const std::string finalName = "series" + description.extension;
    const unsigned long long sequence = ++TemporarySequence;
    const std::string temporaryName =
        ".series." + std::to_string(static_cast<long long>(::getpid())) +
        "." + std::to_string(sequence) + ".tmp";

    FileDescriptor output(::openat(
        directory.get(),
        temporaryName.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
        0640));
    if (!output.valid())
    {
        return false;
    }

    bool stored = writeAll(output.get(), bytes) && ::fsync(output.get()) == 0;
    output.reset();
    if (stored)
    {
        stored = ::renameat(
            directory.get(),
            temporaryName.c_str(),
            directory.get(),
            finalName.c_str()) == 0;
    }
    if (!stored)
    {
        ::unlinkat(directory.get(), temporaryName.c_str(), 0);
        return false;
    }

    const std::string alternateName =
        description.extension == ".png" ? "series.jpg" : "series.png";
    ::unlinkat(directory.get(), alternateName.c_str(), 0);

    if (::fsync(directory.get()) != 0)
    {
        return false;
    }

    destinationPath = (
        configuredRoot /
        backendComponent /
        channelComponent /
        eventComponent /
        finalName).string();
    return true;
}
}

FilesystemSeriesArtworkFallbackMaterializer::
FilesystemSeriesArtworkFallbackMaterializer(
    FilesystemSeriesArtworkFallbackMaterializerConfig config)
    : config_(std::move(config))
{
}

SeriesArtworkFallbackMaterializationResult
FilesystemSeriesArtworkFallbackMaterializer::materialize(
    const SeriesArtworkFallbackMaterializationRequest& request)
{
    SeriesArtworkFallbackMaterializationResult result;
    result.attempted = true;

    if (!request.valid() ||
        config_.allowedSourceRoots.empty() ||
        config_.maximumSourceBytes == 0U ||
        config_.maximumPixels == 0U)
    {
        return result;
    }

    std::string sourcePath;
    if (!EpgArtworkPathPolicy::resolveAllowedPath(
            request.candidate.path,
            config_.allowedSourceRoots,
            sourcePath))
    {
        return result;
    }

    std::vector<unsigned char> bytes;
    if (!readSource(
            sourcePath,
            config_.allowedSourceRoots,
            config_.maximumSourceBytes,
            bytes))
    {
        return result;
    }

    const ImageDescription description = describeImage(bytes);
    if (!acceptableDimensions(description, config_) ||
        request.candidate.width != description.width ||
        request.candidate.height != description.height)
    {
        return result;
    }

    std::string destinationPath;
    if (!atomicallyStore(
            config_.cacheRoot,
            request,
            description,
            bytes,
            destinationPath))
    {
        return result;
    }

    result.stored = true;
    result.artwork.available = true;
    result.artwork.provider = request.candidate.provider;
    result.artwork.origin = EpgScraperArtworkOrigin::ExternalFallback;
    result.artwork.path = std::move(destinationPath);
    result.artwork.width = description.width;
    result.artwork.height = description.height;
    return result;
}

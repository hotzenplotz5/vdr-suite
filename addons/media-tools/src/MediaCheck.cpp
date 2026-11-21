#include "MediaCheck.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace vdrsuite::mediatools {
namespace {
namespace fs = std::filesystem;
constexpr std::size_t kPacketSize = 188;
constexpr std::size_t kReadPackets = 1024;
constexpr std::size_t kMaxSegments = 256;
constexpr std::size_t kPids = 8192;

void require(bool ok, const char* reason)
{
    if (!ok) throw std::runtime_error(reason);
}

struct Fd {
    int value = -1;
    explicit Fd(int fd = -1) : value(fd) {}
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    ~Fd() { if (value >= 0) ::close(value); }
};

void ensureNoSymlinkComponents(const fs::path& root)
{
    fs::path partial;
    for (const auto& part : root) {
        partial /= part;
        require(!fs::is_symlink(fs::symlink_status(partial)),
                "symlinked video root is not allowed");
    }
}

bool segmentName(const std::string& value)
{
    if (value.size() != 8 || value.substr(5) != ".ts") return false;
    for (int i = 0; i != 5; ++i)
        if (value[i] < '0' || value[i] > '9') return false;
    return true;
}

std::string jsonString(const std::string& input)
{
    constexpr char hex[] = "0123456789abcdef";
    std::string output = "\"";
    for (unsigned char c : input) {
        if (c == '"' || c == '\\') {
            output.push_back('\\');
            output.push_back(static_cast<char>(c));
        } else if (c < 0x20) {
            output += "\\u00";
            output.push_back(hex[c >> 4]);
            output.push_back(hex[c & 15]);
        } else {
            output.push_back(static_cast<char>(c));
        }
    }
    output += '"';
    return output;
}

struct Continuity {
    std::array<int, kPids> last{};
    std::array<bool, kPids> reset{};
    Continuity() { last.fill(-1); reset.fill(false); }
};

void inspectPacket(const unsigned char* p,
                   MediaCheckReport& report, Continuity& counters)
{
    ++report.packets;
    if (p[0] != 0x47) {
        ++report.syncErrors;
        return; // Keep fixed byte alignment; never rewrite damaged packets.
    }
    if (p[1] & 0x80) ++report.transportErrors;

    const unsigned pid = (static_cast<unsigned>(p[1] & 0x1f) << 8) | p[2];
    const unsigned control = (p[3] >> 4) & 3;
    const int cc = p[3] & 15;
    if (control == 0) {
        ++report.adaptationErrors;
        return;
    }

    bool discontinuity = false;
    if (control == 2 || control == 3) {
        const unsigned size = p[4];
        if (size > 183) {
            ++report.adaptationErrors;
            counters.last[pid] = -1;
            return;
        }
        if (size != 0 && (p[5] & 0x80) != 0) {
            ++report.discontinuities;
            discontinuity = true;
        }
        // Adaptation-only packets must fill all 184 bytes following the
        // TS header: length byte plus exactly 183 adaptation bytes.
        if (control == 2 && size != 183) {
            ++report.adaptationErrors;
            counters.last[pid] = -1;
            return;
        }
        // A payload-bearing packet must leave at least one byte for payload.
        if (control == 3 && size >= 183) {
            ++report.adaptationErrors;
            counters.last[pid] = -1;
            return;
        }
    }
    if (pid == 0x1fff) return; // Null PID does not have a useful continuity counter.
    if (discontinuity) counters.reset[pid] = true;
    if (control == 2 && counters.last[pid] >= 0 &&
        !counters.reset[pid] && cc != counters.last[pid])
        ++report.continuityWarnings; // No payload: counter must not advance.
    if (control == 1 || control == 3) {
        if (counters.last[pid] >= 0 && !counters.reset[pid] &&
            cc != ((counters.last[pid] + 1) & 15))
            ++report.continuityWarnings;
        counters.last[pid] = cc;
        counters.reset[pid] = false;
    }
}

std::vector<std::string> listSegments(int directory)
{
    Fd iteratorFd(::dup(directory));
    require(iteratorFd.value >= 0, "cannot duplicate recording directory");
    DIR* raw = ::fdopendir(iteratorFd.value);
    require(raw != nullptr, "cannot list recording directory");
    iteratorFd.value = -1;
    std::vector<std::string> segments;
    errno = 0;
    while (struct dirent* entry = ::readdir(raw)) {
        const std::string name(entry->d_name);
        if (segmentName(name)) {
            segments.push_back(name);
            require(segments.size() <= kMaxSegments, "too many TS segments");
        }
        errno = 0;
    }
    const int scanError = errno;
    ::closedir(raw);
    require(scanError == 0, "recording directory listing failed");
    std::sort(segments.begin(), segments.end());
    require(!segments.empty(), "recording has no numbered TS segments");
    for (std::size_t i = 0; i < segments.size(); ++i) {
        char expected[] = "00000.ts";
        std::size_t sequence = i + 1; // bounded by kMaxSegments
        for (int digit = 4; digit >= 0; --digit) {
            expected[digit] = static_cast<char>('0' + sequence % 10);
            sequence /= 10;
        }
        require(segments[i] == expected, "recording TS segment sequence has a gap");
    }
    return segments;
}

void scanSegment(int directory, const std::string& name,
                 MediaCheckReport& report, Continuity& counters)
{
    Fd file(::openat(directory, name.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    require(file.value >= 0, "cannot open regular TS segment");
    struct stat first{}, last{};
    require(::fstat(file.value, &first) == 0 &&
            S_ISREG(first.st_mode) && first.st_size > 0,
            "TS segment is empty or not a regular file");
    const auto length = static_cast<std::uint64_t>(first.st_size);
    require(report.bytes <= std::numeric_limits<std::uint64_t>::max() - length,
            "recording size overflow");
    report.bytes += length;
    report.trailingBytes += length % kPacketSize;
    std::array<unsigned char, kPacketSize * kReadPackets> bytes{};
    const std::uint64_t completeBytes = length - length % kPacketSize;
    for (std::uint64_t offset = 0; offset != completeBytes;) {
        const std::size_t wanted = static_cast<std::size_t>(
            std::min<std::uint64_t>(completeBytes - offset, bytes.size()));
        std::size_t got = 0;
        while (got < wanted) {
            const ssize_t n = ::pread(file.value, bytes.data() + got,
                wanted - got, static_cast<off_t>(offset + got));
            if (n < 0 && errno == EINTR) continue;
            require(n > 0, "TS segment changed or read failed");
            got += static_cast<std::size_t>(n);
        }
        for (std::size_t i = 0; i < wanted; i += kPacketSize)
            inspectPacket(bytes.data() + i, report, counters);
        offset += wanted;
    }
    require(::fstat(file.value, &last) == 0 &&
            last.st_dev == first.st_dev &&
            last.st_ino == first.st_ino &&
            last.st_size == first.st_size &&
            last.st_mtim.tv_sec == first.st_mtim.tv_sec &&
            last.st_mtim.tv_nsec == first.st_mtim.tv_nsec,
            "TS segment changed during check; result unknown");
}

bool indexAvailable(int directory)
{
    for (const char* name : {"index", "index.vdr"}) {
        struct stat st{};
        if (::fstatat(directory, name, &st, AT_SYMLINK_NOFOLLOW) == 0 &&
            S_ISREG(st.st_mode) && st.st_size > 0)
            return true;
    }
    return false;
}

bool isRelevantSidecar(const std::string& name)
{
    if (name == "index" || name == "index.vdr" ||
        name == "info" || name == "info.vdr" ||
        name == "marks" || name == "marks.vdr" ||
        name == "resume" || name == "resume.vdr")
        return true;
    const auto extension = fs::path(name).extension().string();
    return extension == ".srt" || extension == ".nfo";
}

// Classify actual on-disk sidecars, not names supplied by an API user.
// lstat-equivalent fstatat never follows a link outside the .rec directory.
void scanSidecars(int directory, MediaCheckReport& report)
{
    Fd listingFd(::openat(directory, ".", O_RDONLY | O_DIRECTORY |
                          O_NOFOLLOW | O_CLOEXEC));
    require(listingFd.value >= 0, "cannot list recording sidecars");
    DIR* raw = ::fdopendir(listingFd.value);
    require(raw != nullptr, "cannot scan recording sidecars");
    listingFd.value = -1;
    std::vector<MediaSidecarReport> found;
    errno = 0;
    while (struct dirent* entry = ::readdir(raw)) {
        const std::string name(entry->d_name);
        if (name != "." && name != ".." && isRelevantSidecar(name)) {
            if (found.size() >= 64) {
                ::closedir(raw);
                throw std::runtime_error("too many recording sidecars");
            }
            struct stat status{};
            MediaSidecarReport sidecar;
            sidecar.name = name;
            if (::fstatat(directory, name.c_str(), &status,
                          AT_SYMLINK_NOFOLLOW) != 0 ||
                !S_ISREG(status.st_mode) || status.st_size < 0) {
                sidecar.status = "errors";
                ++report.sidecarErrors;
            } else {
                sidecar.bytes = static_cast<std::uint64_t>(status.st_size);
                const bool requiredContent = name == "index" ||
                    name == "index.vdr" || name == "info" ||
                    name == "info.vdr" ||
                    fs::path(name).extension() == ".srt";
                if (requiredContent && sidecar.bytes == 0) {
                    sidecar.status = "warnings";
                    ++report.sidecarWarnings;
                } else {
                    sidecar.status = "ok";
                }
            }
            found.push_back(std::move(sidecar));
        }
        errno = 0;
    }
    const int listError = errno;
    ::closedir(raw);
    require(listError == 0, "cannot finish scanning sidecars");
    std::sort(found.begin(), found.end(),
        [](const MediaSidecarReport& a, const MediaSidecarReport& b) {
            return a.name < b.name;
        });
    report.sidecarReports = std::move(found);
}

} // namespace

MediaCheckReport checkRecordingInternal(
    const fs::path& videoRoot, const fs::path& recordingRelative,
    MediaCheckDecoder* decoder, MediaCheckProber* prober)
{
    require(videoRoot.is_absolute() && videoRoot.lexically_normal() == videoRoot,
            "video root must be absolute and normalized");
    require(!recordingRelative.empty() && recordingRelative.is_relative() &&
            recordingRelative.lexically_normal() == recordingRelative &&
            recordingRelative.extension() == ".rec",
            "recording must be a normalized relative .rec directory");
    ensureNoSymlinkComponents(videoRoot);
    Fd current(::open(videoRoot.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    require(current.value >= 0, "cannot open configured video root");
    std::size_t depth = 0;
    for (const auto& component : recordingRelative) {
        const std::string name = component.string();
        require(!name.empty() && name != "." && name != ".." &&
                name.find('/') == std::string::npos && ++depth <= 64,
                "invalid recording directory component");
        const int next = ::openat(current.value, name.c_str(),
            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        require(next >= 0, "recording directory absent or symlinked");
        ::close(current.value);
        current.value = next;
    }
    require(depth >= 2, "recording must be below a title directory");
    const auto segments = listSegments(current.value);
    MediaCheckReport result;
    result.recording = recordingRelative.generic_string();
    result.segments = segments.size();
    result.indexPresent = indexAvailable(current.value);
    scanSidecars(current.value, result);
    Continuity counters;
    for (const auto& segment : segments) {
        MediaCheckSegmentReport detail;
        detail.name = segment;
        const auto before = result;
        scanSegment(current.value, segment, result, counters);
        detail.bytes = result.bytes - before.bytes;
        detail.packets = result.packets - before.packets;
        detail.syncErrors = result.syncErrors - before.syncErrors;
        detail.transportErrors = result.transportErrors - before.transportErrors;
        detail.adaptationErrors = result.adaptationErrors - before.adaptationErrors;
        detail.trailingBytes = result.trailingBytes - before.trailingBytes;
        detail.continuityWarnings = result.continuityWarnings - before.continuityWarnings;
        detail.discontinuities = result.discontinuities - before.discontinuities;
        result.segmentReports.push_back(std::move(detail));
    }
    if (prober != nullptr)
        result.ffprobe = prober->probe(current.value, segments);
    if (decoder != nullptr)
        result.decode = decoder->decode(current.value, segments);
    return result;
}

MediaCheckReport checkRecordingTransportStream(
    const fs::path& videoRoot, const fs::path& recordingRelative)
{
    return checkRecordingInternal(videoRoot, recordingRelative, nullptr, nullptr);
}

MediaCheckReport checkRecordingWithDecoder(
    const fs::path& videoRoot, const fs::path& recordingRelative,
    MediaCheckDecoder& decoder)
{
    return checkRecordingInternal(videoRoot, recordingRelative, &decoder, nullptr);
}

MediaCheckReport checkRecordingWithProber(
    const fs::path& videoRoot, const fs::path& recordingRelative,
    MediaCheckProber& prober, MediaCheckDecoder* decoder)
{
    return checkRecordingInternal(videoRoot, recordingRelative, decoder, &prober);
}

std::string MediaCheckSegmentReport::status() const
{
    if (syncErrors || transportErrors || adaptationErrors || trailingBytes)
        return "errors";
    if (continuityWarnings || discontinuities)
        return "warnings";
    return "ok";
}

std::string MediaCheckReport::status() const
{
    if (decode && decode->status == "incomplete") return "incomplete";
    if (decode && decode->status == "errors") return "errors";
    if (syncErrors || transportErrors || adaptationErrors || trailingBytes ||
        sidecarErrors)
        return "errors";
    // Probe failures are an incomplete analysis, never verified "ok".
    // Preserve positive structural error findings over probe unavailability.
    if (ffprobe && ffprobe->status != "ok") return "incomplete";
    if (continuityWarnings || discontinuities || !indexPresent ||
        sidecarWarnings)
        return "warnings";
    return "ok";
}

std::string MediaCheckReport::json() const
{
    std::ostringstream output;
    output << "{\"schemaVersion\":1,\"recording\":" << jsonString(recording)
        << ",\"status\":" << jsonString(status())
        << ",\"segments\":" << segments
        << ",\"bytes\":" << bytes
        << ",\"packets\":" << packets
        << ",\"syncErrors\":" << syncErrors
        << ",\"transportErrors\":" << transportErrors
        << ",\"adaptationErrors\":" << adaptationErrors
        << ",\"trailingBytes\":" << trailingBytes
        << ",\"continuityWarnings\":" << continuityWarnings
        << ",\"discontinuities\":" << discontinuities
        << ",\"indexPresent\":" << (indexPresent ? "true" : "false")
        << ",\"sidecarErrors\":" << sidecarErrors
        << ",\"sidecarWarnings\":" << sidecarWarnings
        << ",\"analysis\":"
        << jsonString(decode ? "ts_structure_and_ffmpeg_decode" : "ts_structure_only")
        << ",\"readOnly\":true";
    output << ",\"segmentReports\":[";
    for (std::size_t index = 0; index < segmentReports.size(); ++index) {
        const auto& segment = segmentReports[index];
        if (index != 0) output << ",";
        output << "{\"name\":" << jsonString(segment.name)
            << ",\"status\":" << jsonString(segment.status())
            << ",\"bytes\":" << segment.bytes
            << ",\"packets\":" << segment.packets
            << ",\"syncErrors\":" << segment.syncErrors
            << ",\"transportErrors\":" << segment.transportErrors
            << ",\"adaptationErrors\":" << segment.adaptationErrors
            << ",\"trailingBytes\":" << segment.trailingBytes
            << ",\"continuityWarnings\":" << segment.continuityWarnings
            << ",\"discontinuities\":" << segment.discontinuities
            << "}";
    }
    output << "],\"sidecarReports\":[";
    for (std::size_t index = 0; index < sidecarReports.size(); ++index) {
        const auto& sidecar = sidecarReports[index];
        if (index != 0) output << ",";
        output << "{\"name\":" << jsonString(sidecar.name)
            << ",\"status\":" << jsonString(sidecar.status)
            << ",\"bytes\":" << sidecar.bytes << "}";
    }
    output << "]";
    if (decode) {
        output << ",\"decoder\":{\"status\":" << jsonString(decode->status)
            << ",\"exitCode\":" << decode->exitCode
            << ",\"timedOut\":" << (decode->timedOut ? "true" : "false")
            << ",\"streamedBytes\":" << decode->streamedBytes
            << ",\"diagnostic\":" << jsonString(decode->diagnostic)
            << "}";
    }
    if (ffprobe) {
        output << ",\"ffprobe\":{\"status\":" << jsonString(ffprobe->status)
            << ",\"details\":" << jsonString(ffprobe->details)
            << ",\"diagnostic\":" << jsonString(ffprobe->diagnostic)
            << "}";
    }
    output << "}";
    return output.str();
}

} // namespace vdrsuite::mediatools

#include "MediaCheckFfmpeg.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace vdrsuite::mediatools {
namespace {

struct Fd {
    int fd = -1;
    explicit Fd(int value = -1) : fd(value) {}
    ~Fd() { if (fd >= 0) ::close(fd); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    Fd(Fd&& rhs) noexcept : fd(std::exchange(rhs.fd, -1)) {}
    Fd& operator=(Fd&& rhs) noexcept {
        if (this != &rhs) {
            if (fd >= 0) ::close(fd);
            fd = std::exchange(rhs.fd, -1);
        }
        return *this;
    }
};

struct Input {
    Fd fd;
    struct stat before{};
    std::uint64_t offset = 0;
};

bool unchanged(const struct stat& a, const struct stat& b)
{
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino &&
        a.st_size == b.st_size && a.st_mtim.tv_sec == b.st_mtim.tv_sec &&
        a.st_mtim.tv_nsec == b.st_mtim.tv_nsec;
}

void closeInput(int fd) { ::shutdown(fd, SHUT_WR); }

MediaDecodeResult incomplete(const char* reason)
{
    MediaDecodeResult result;
    result.diagnostic = reason;
    return result;
}

} // namespace

FfmpegMediaCheckDecoder::FfmpegMediaCheckDecoder(
    std::chrono::seconds deadline, std::string trustedExecutable)
    : deadline_(deadline), executable_(std::move(trustedExecutable))
{
    if (deadline_.count() < 1 || deadline_.count() > 3600 ||
        executable_.empty() || executable_[0] != '/')
        throw std::invalid_argument("invalid internal FFmpeg decoder configuration");
}

MediaDecodeResult FfmpegMediaCheckDecoder::decode(
    int directory, const std::vector<std::string>& segments)
{
    if (directory < 0 || segments.empty() || segments.size() > 256)
        return incomplete("invalid recording descriptor or segment list");

    // Open exclusively from the already validated recording directory.
    // FFmpeg receives bytes on stdin, never recording filenames or file FDs.
    std::vector<Input> sources;
    sources.reserve(segments.size());
    for (const auto& name : segments) {
        if (name.size() != 8 || name.substr(5) != ".ts")
            return incomplete("invalid TS segment name");
        for (int i = 0; i < 5; ++i)
            if (name[static_cast<std::size_t>(i)] < '0' ||
                name[static_cast<std::size_t>(i)] > '9')
                return incomplete("invalid TS segment number");
        Input input;
        input.fd = Fd(::openat(directory, name.c_str(),
            O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
        if (input.fd.fd < 0 ||
            ::fstat(input.fd.fd, &input.before) != 0 ||
            !S_ISREG(input.before.st_mode) || input.before.st_size <= 0)
            return incomplete("recording segment unavailable for decode");
        sources.push_back(std::move(input));
    }

    int data[2]{-1, -1};
    int errors[2]{-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, data) != 0)
        return incomplete("cannot create FFmpeg input pipe");
    Fd parentData(data[0]), childData(data[1]);
    if (::pipe2(errors, O_CLOEXEC) != 0)
        return incomplete("cannot create FFmpeg diagnostics pipe");
    Fd readErrors(errors[0]), writeErrors(errors[1]);
    const pid_t pid = ::fork();
    if (pid < 0) return incomplete("cannot start FFmpeg");
    if (pid == 0) {
        // Software decoding only; no shell, no input pathname and no output
        // file. The null muxer discards all decoded frames.
        if (::setsid() < 0 ||
            ::dup2(childData.fd, STDIN_FILENO) < 0 ||
            ::dup2(writeErrors.fd, STDERR_FILENO) < 0)
            ::_exit(127);
        const int nullfd = ::open("/dev/null", O_WRONLY | O_CLOEXEC);
        if (nullfd < 0 || ::dup2(nullfd, STDOUT_FILENO) < 0)
            ::_exit(127);
        ::close(nullfd);
        ::close(parentData.fd);
        ::close(readErrors.fd);
        ::close(childData.fd);
        ::close(writeErrors.fd);
        const char* args[] = {
            executable_.c_str(), "-hide_banner", "-nostdin",
            "-loglevel", "error", "-xerror", "-err_detect", "explode",
            "-threads", "1", "-f", "mpegts", "-i", "pipe:0",
            "-map", "0:v?", "-map", "0:a?", "-sn", "-dn",
            "-f", "null", "-", nullptr
        };
        ::execv(executable_.c_str(), const_cast<char* const*>(args));
        ::_exit(127);
    }

    childData = Fd();
    writeErrors = Fd();
    const int flags = ::fcntl(readErrors.fd, F_GETFL);
    if (flags >= 0) ::fcntl(readErrors.fd, F_SETFL, flags | O_NONBLOCK);

    MediaDecodeResult result;
    const auto deadline = std::chrono::steady_clock::now() + deadline_;
    std::array<char, 65536> buffer{};
    std::size_t buffered = 0, sent = 0, sourceIndex = 0;
    bool inputClosed = false, diagnosticsClosed = false, childExited = false;
    bool sourceChanged = false, pipeError = false;
    int childStatus = 0;

    while (true) {
        if (!childExited) {
            const pid_t checked = ::waitpid(pid, &childStatus, WNOHANG);
            if (checked == pid) childExited = true;
            else if (checked < 0 && errno != EINTR) {
                pipeError = true;
                break;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            result.timedOut = true;
            break;
        }
        if (childExited && !inputClosed) {
            closeInput(parentData.fd);
            inputClosed = true;
        }
        if (!inputClosed && sent == buffered) {
            sent = buffered = 0;
            while (sourceIndex < sources.size()) {
                Input& source = sources[sourceIndex];
                if (source.offset == static_cast<std::uint64_t>(source.before.st_size)) {
                    struct stat after{};
                    if (::fstat(source.fd.fd, &after) != 0 ||
                        !unchanged(source.before, after)) sourceChanged = true;
                    ++sourceIndex;
                    continue;
                }
                const std::size_t wanted = static_cast<std::size_t>(
                    std::min<std::uint64_t>(
                        buffer.size(), static_cast<std::uint64_t>(
                            source.before.st_size) - source.offset));
                const ssize_t n = ::pread(source.fd.fd, buffer.data(), wanted,
                                          static_cast<off_t>(source.offset));
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) { sourceChanged = true; break; }
                source.offset += static_cast<std::uint64_t>(n);
                buffered = static_cast<std::size_t>(n);
                break;
            }
            if (sourceChanged) { pipeError = true; break; }
            if (sourceIndex == sources.size() && buffered == 0) {
                closeInput(parentData.fd);
                inputClosed = true;
            }
        }

        if (childExited && diagnosticsClosed) break;
        struct pollfd pollers[2]{};
        pollers[0].fd = inputClosed ? -1 : parentData.fd;
        pollers[0].events = POLLOUT;
        pollers[1].fd = diagnosticsClosed ? -1 : readErrors.fd;
        pollers[1].events = POLLIN;
        const int p = ::poll(pollers, 2, 50);
        if (p < 0 && errno == EINTR) continue;
        if (p < 0) { pipeError = true; break; }

        if (!diagnosticsClosed &&
            (pollers[1].revents & (POLLIN | POLLHUP | POLLERR))) {
            std::array<char, 1024> chunk{};
            while (true) {
                const ssize_t n = ::read(readErrors.fd, chunk.data(), chunk.size());
                if (n > 0) {
                    if (result.diagnostic.size() < 4096)
                        result.diagnostic.append(chunk.data(),
                            std::min<std::size_t>(static_cast<std::size_t>(n),
                                4096 - result.diagnostic.size()));
                } else if (n == 0) {
                    diagnosticsClosed = true;
                    break;
                } else if (errno == EINTR) {
                    continue;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    diagnosticsClosed = true;
                    break;
                }
            }
        }
        if (!inputClosed && (pollers[0].revents & (POLLOUT | POLLHUP | POLLERR))) {
            if (buffered > sent) {
                const ssize_t n = ::send(parentData.fd, buffer.data() + sent,
                    buffered - sent, MSG_NOSIGNAL | MSG_DONTWAIT);
                if (n > 0) {
                    sent += static_cast<std::size_t>(n);
                    result.streamedBytes += static_cast<std::uint64_t>(n);
                } else if (n < 0 && errno != EINTR &&
                           errno != EAGAIN && errno != EWOULDBLOCK) {
                    closeInput(parentData.fd);
                    inputClosed = true;
                }
            } else if (pollers[0].revents & (POLLHUP | POLLERR)) {
                closeInput(parentData.fd);
                inputClosed = true;
            }
        }
    }

    // Failure and timeout kill only the detached FFmpeg process group.
    // Successful processes are reaped without affecting any VDR service.
    if (!childExited) {
        ::kill(-pid, SIGKILL);
        ::kill(pid, SIGKILL);
        while (::waitpid(pid, &childStatus, 0) < 0 && errno == EINTR) {}
    }
    if (result.timedOut) {
        result.status = "incomplete";
        result.diagnostic = "FFmpeg decoding exceeded the configured wall-time limit";
        return result;
    }
    if (sourceChanged || pipeError) {
        result.status = "incomplete";
        result.diagnostic = "recording input changed or decoder communication failed";
        return result;
    }
    result.exitCode = WIFEXITED(childStatus) ? WEXITSTATUS(childStatus) : -1;
    if (result.exitCode == 127) {
        result.status = "incomplete";
        result.diagnostic = "FFmpeg executable unavailable";
    } else if (result.exitCode == 0 && inputClosed && sourceIndex == sources.size()) {
        result.status = "ok";
    } else if (result.exitCode == 0) {
        result.status = "incomplete";
        result.diagnostic = "FFmpeg stopped before receiving the complete recording";
    } else {
        result.status = "errors";
        if (result.diagnostic.empty()) result.diagnostic = "FFmpeg reported a decoding error";
    }
    return result;
}

} // namespace vdrsuite::mediatools

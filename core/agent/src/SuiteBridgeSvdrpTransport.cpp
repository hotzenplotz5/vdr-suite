#include "SuiteBridgeSvdrpTransport.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

enum class IoStatus
{
    Ready,
    Timeout,
    Failed
};

class ScopedFd
{
public:
    explicit ScopedFd(int fd = -1) : fd_(fd) {}
    ~ScopedFd() { if (fd_ >= 0) close(fd_); }

    ScopedFd(const ScopedFd&) = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;

    ScopedFd(ScopedFd&& other) noexcept : fd_(other.release()) {}
    ScopedFd& operator=(ScopedFd&& other) noexcept
    {
        if (this != &other)
        {
            reset(other.release());
        }
        return *this;
    }

    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }

    int release()
    {
        const int value = fd_;
        fd_ = -1;
        return value;
    }

    void reset(int fd = -1)
    {
        if (fd_ >= 0)
        {
            close(fd_);
        }
        fd_ = fd;
    }

private:
    int fd_;
};

struct ParsedReply
{
    IoStatus status = IoStatus::Failed;
    int code = 0;
    std::string payload;
    std::string diagnostic;
};

SuiteBridgeCommandReply failure(
    SuiteBridgeTransportStatus status,
    std::string diagnostic)
{
    SuiteBridgeCommandReply reply;
    reply.transportStatus = status;
    reply.diagnostic = std::move(diagnostic);
    return reply;
}

std::string normalizedHost(const std::string& host)
{
    return host == "localhost" ? "127.0.0.1" : host;
}

const char* commandText(SuiteBridgeLocalCommand command)
{
    switch (command)
    {
        case SuiteBridgeLocalCommand::DiscoverSchema1:
            return "PLUG suitebridge CAPS 1\r\n";
        case SuiteBridgeLocalCommand::Snapshot:
            return "PLUG suitebridge SNAP\r\n";
    }
    return nullptr;
}

bool safeToken(const std::string& value)
{
    if (value.empty() || value.size() > 255)
    {
        return false;
    }

    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch > 0x20 && ch != 0x7f;
    });
}

TimePoint boundedDeadline(
    TimePoint operationDeadline,
    std::chrono::milliseconds phaseTimeout)
{
    return std::min(operationDeadline, Clock::now() + phaseTimeout);
}

int pollTimeoutMilliseconds(TimePoint deadline)
{
    const auto now = Clock::now();
    if (now >= deadline)
    {
        return 0;
    }

    const auto remaining =
        std::chrono::duration_cast<std::chrono::microseconds>(deadline - now)
            .count();
    const auto rounded = (remaining + 999) / 1000;
    return rounded > INT_MAX
        ? INT_MAX
        : static_cast<int>(std::max<long long>(1, rounded));
}

IoStatus waitForSocket(int fd, short events, TimePoint deadline)
{
    while (true)
    {
        const int timeout = pollTimeoutMilliseconds(deadline);
        if (timeout <= 0)
        {
            return IoStatus::Timeout;
        }

        pollfd descriptor{};
        descriptor.fd = fd;
        descriptor.events = events;

        const int result = poll(&descriptor, 1, timeout);
        if (result == 0)
        {
            return IoStatus::Timeout;
        }
        if (result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return IoStatus::Failed;
        }
        if ((descriptor.revents & events) != 0 ||
            (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
            return IoStatus::Ready;
        }
    }
}

bool configureSocket(int fd)
{
    const int statusFlags = fcntl(fd, F_GETFL, 0);
    if (statusFlags < 0 ||
        fcntl(fd, F_SETFL, statusFlags | O_NONBLOCK) != 0)
    {
        return false;
    }

    const int descriptorFlags = fcntl(fd, F_GETFD, 0);
    return descriptorFlags >= 0 &&
        fcntl(fd, F_SETFD, descriptorFlags | FD_CLOEXEC) == 0;
}

SuiteBridgeTransportStatus connectSocket(
    const SuiteBridgeSvdrpTransportConfig& config,
    TimePoint operationDeadline,
    ScopedFd& connected,
    std::string& diagnostic)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

    addrinfo* resolved = nullptr;
    const std::string host = normalizedHost(config.host);
    const std::string port = std::to_string(config.port);

    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &resolved) != 0)
    {
        diagnostic = "SVDRP host must be localhost or a numeric address";
        return SuiteBridgeTransportStatus::Failed;
    }

    bool timedOut = false;
    for (addrinfo* current = resolved; current; current = current->ai_next)
    {
        ScopedFd candidate(socket(
            current->ai_family,
            current->ai_socktype,
            current->ai_protocol));
        if (!candidate.valid() || !configureSocket(candidate.get()))
        {
            continue;
        }

        if (connect(
                candidate.get(),
                current->ai_addr,
                current->ai_addrlen) == 0)
        {
            connected = std::move(candidate);
            freeaddrinfo(resolved);
            return SuiteBridgeTransportStatus::Success;
        }

        if (errno != EINPROGRESS && errno != EWOULDBLOCK)
        {
            continue;
        }

        const IoStatus waitStatus = waitForSocket(
            candidate.get(),
            POLLOUT,
            boundedDeadline(operationDeadline, config.connectTimeout));
        if (waitStatus == IoStatus::Timeout)
        {
            timedOut = true;
            continue;
        }
        if (waitStatus == IoStatus::Failed)
        {
            continue;
        }

        int socketError = 0;
        socklen_t socketErrorSize = sizeof(socketError);
        if (getsockopt(
                candidate.get(),
                SOL_SOCKET,
                SO_ERROR,
                &socketError,
                &socketErrorSize) != 0)
        {
            continue;
        }
        if (socketError == 0)
        {
            connected = std::move(candidate);
            freeaddrinfo(resolved);
            return SuiteBridgeTransportStatus::Success;
        }
        if (socketError == ETIMEDOUT)
        {
            timedOut = true;
        }
    }

    freeaddrinfo(resolved);
    if (timedOut)
    {
        diagnostic = "SVDRP connect timed out";
        return SuiteBridgeTransportStatus::Timeout;
    }

    diagnostic = "SVDRP endpoint connection failed";
    return SuiteBridgeTransportStatus::Failed;
}

IoStatus sendAll(int fd, const std::string& data, TimePoint deadline)
{
    std::size_t offset = 0;
    while (offset < data.size())
    {
        const IoStatus waitStatus = waitForSocket(fd, POLLOUT, deadline);
        if (waitStatus != IoStatus::Ready)
        {
            return waitStatus;
        }

        const ssize_t written = send(
            fd,
            data.data() + offset,
            data.size() - offset,
#ifdef MSG_NOSIGNAL
            MSG_NOSIGNAL
#else
            0
#endif
        );
        if (written > 0)
        {
            offset += static_cast<std::size_t>(written);
            continue;
        }
        if (written < 0 &&
            (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
        {
            continue;
        }
        return IoStatus::Failed;
    }
    return IoStatus::Ready;
}

IoStatus readLine(
    int fd,
    std::string& pending,
    std::size_t& receivedBytes,
    std::size_t maximumBytes,
    TimePoint deadline,
    std::string& line)
{
    while (true)
    {
        const std::size_t newline = pending.find('\n');
        if (newline != std::string::npos)
        {
            line = pending.substr(0, newline);
            pending.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            return IoStatus::Ready;
        }

        if (receivedBytes >= maximumBytes)
        {
            return IoStatus::Failed;
        }

        const IoStatus waitStatus = waitForSocket(fd, POLLIN, deadline);
        if (waitStatus != IoStatus::Ready)
        {
            return waitStatus;
        }

        char buffer[512];
        const std::size_t requested =
            std::min(maximumBytes - receivedBytes, sizeof(buffer));
        const ssize_t received = recv(fd, buffer, requested, 0);
        if (received > 0)
        {
            pending.append(buffer, static_cast<std::size_t>(received));
            receivedBytes += static_cast<std::size_t>(received);
            continue;
        }
        if (received < 0 &&
            (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
        {
            continue;
        }
        return IoStatus::Failed;
    }
}

bool parseReplyLine(
    const std::string& line,
    int& code,
    char& separator,
    std::string& text)
{
    if (line.size() < 4 ||
        line[0] < '0' || line[0] > '9' ||
        line[1] < '0' || line[1] > '9' ||
        line[2] < '0' || line[2] > '9')
    {
        return false;
    }

    separator = line[3];
    if (separator != ' ' && separator != '-')
    {
        return false;
    }

    code = (line[0] - '0') * 100 +
        (line[1] - '0') * 10 +
        (line[2] - '0');
    text = line.substr(4);
    return true;
}

ParsedReply readReply(int fd, std::size_t maximumBytes, TimePoint deadline)
{
    ParsedReply result;
    std::string pending;
    std::size_t receivedBytes = 0;
    bool codeSeen = false;

    for (std::size_t lineIndex = 0;
         lineIndex < SuiteBridgeSvdrpTransport::MaximumReplyLines;
         ++lineIndex)
    {
        std::string line;
        const IoStatus lineStatus = readLine(
            fd,
            pending,
            receivedBytes,
            maximumBytes,
            deadline,
            line);
        if (lineStatus == IoStatus::Timeout)
        {
            result.status = IoStatus::Timeout;
            result.diagnostic = "SVDRP reply timed out";
            return result;
        }
        if (lineStatus == IoStatus::Failed)
        {
            result.status = IoStatus::Failed;
            result.diagnostic = receivedBytes >= maximumBytes
                ? "SVDRP reply exceeds bounded size"
                : "SVDRP connection closed before complete reply";
            return result;
        }

        int code = 0;
        char separator = 0;
        std::string text;
        if (!parseReplyLine(line, code, separator, text))
        {
            result.status = IoStatus::Failed;
            result.diagnostic = "malformed SVDRP reply line";
            return result;
        }

        if (!codeSeen)
        {
            result.code = code;
            codeSeen = true;
        }
        else if (code != result.code)
        {
            result.status = IoStatus::Failed;
            result.diagnostic = "inconsistent SVDRP multiline reply code";
            return result;
        }

        if (!result.payload.empty())
        {
            result.payload.push_back('\n');
        }
        result.payload += text;
        if (result.payload.size() > maximumBytes)
        {
            result.status = IoStatus::Failed;
            result.diagnostic = "SVDRP payload exceeds bounded size";
            return result;
        }

        if (separator == ' ')
        {
            result.status = IoStatus::Ready;
            return result;
        }
    }

    result.status = IoStatus::Failed;
    result.diagnostic = "SVDRP reply exceeds bounded line count";
    return result;
}

}

SuiteBridgeSvdrpTransport::SuiteBridgeSvdrpTransport(
    SuiteBridgeSvdrpTransportConfig config)
    : config_(std::move(config))
{
}

SuiteBridgeCommandReply SuiteBridgeSvdrpTransport::execute(
    SuiteBridgeLocalCommand command)
{
    const char* requestText = commandText(command);
    if (!requestText)
    {
        return failure(
            SuiteBridgeTransportStatus::Failed,
            "unsupported Suite Bridge local command");
    }
    return executeRequest(requestText);
}

SuiteBridgeArtworkCommandReply SuiteBridgeSvdrpTransport::requestArtwork(
    const std::string& channelId,
    const std::string& eventId)
{
    SuiteBridgeArtworkCommandReply artworkReply;
    if (!safeToken(channelId) || !safeToken(eventId))
    {
        return artworkReply;
    }

    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge ARTW " + channelId + " " + eventId + "\r\n");
    artworkReply.transportSucceeded = reply.transportSucceeded();
    artworkReply.replyCode = reply.replyCode;
    artworkReply.payload = reply.payload;
    return artworkReply;
}

SuiteBridgeCommandReply SuiteBridgeSvdrpTransport::executeRequest(
    const std::string& requestText)
{
    if (config_.host.empty())
    {
        return failure(
            SuiteBridgeTransportStatus::Unavailable,
            "SVDRP transport is not configured");
    }
    if (config_.host.size() > 255 ||
        config_.port <= 0 || config_.port > 65535 ||
        config_.connectTimeout.count() <= 0 ||
        config_.ioTimeout.count() <= 0 ||
        config_.operationTimeout.count() <= 0)
    {
        return failure(
            SuiteBridgeTransportStatus::Failed,
            "invalid SVDRP transport configuration");
    }

    const TimePoint operationDeadline =
        Clock::now() + config_.operationTimeout;
    ScopedFd socketFd;
    std::string connectDiagnostic;
    const SuiteBridgeTransportStatus connectStatus = connectSocket(
        config_, operationDeadline, socketFd, connectDiagnostic);
    if (connectStatus != SuiteBridgeTransportStatus::Success)
    {
        return failure(connectStatus, std::move(connectDiagnostic));
    }

    const ParsedReply greeting = readReply(
        socketFd.get(),
        MaximumGreetingBytes,
        boundedDeadline(operationDeadline, config_.ioTimeout));
    if (greeting.status == IoStatus::Timeout)
    {
        return failure(
            SuiteBridgeTransportStatus::Timeout,
            "SVDRP greeting timed out");
    }
    if (greeting.status != IoStatus::Ready || greeting.code != 220)
    {
        return failure(
            SuiteBridgeTransportStatus::Failed,
            greeting.status == IoStatus::Ready
                ? "unexpected SVDRP greeting code"
                : greeting.diagnostic);
    }

    const IoStatus sendStatus = sendAll(
        socketFd.get(),
        requestText,
        boundedDeadline(operationDeadline, config_.ioTimeout));
    if (sendStatus == IoStatus::Timeout)
    {
        return failure(
            SuiteBridgeTransportStatus::Timeout,
            "SVDRP request send timed out");
    }
    if (sendStatus != IoStatus::Ready)
    {
        return failure(
            SuiteBridgeTransportStatus::Failed,
            "SVDRP request send failed");
    }

    ParsedReply commandReply = readReply(
        socketFd.get(),
        MaximumReplyBytes,
        boundedDeadline(operationDeadline, config_.ioTimeout));
    if (commandReply.status == IoStatus::Timeout)
    {
        return failure(
            SuiteBridgeTransportStatus::Timeout,
            std::move(commandReply.diagnostic));
    }
    if (commandReply.status != IoStatus::Ready)
    {
        return failure(
            SuiteBridgeTransportStatus::Failed,
            std::move(commandReply.diagnostic));
    }

    SuiteBridgeCommandReply reply;
    reply.transportStatus = SuiteBridgeTransportStatus::Success;
    reply.replyCode = commandReply.code;
    reply.payload = std::move(commandReply.payload);
    return reply;
}

}

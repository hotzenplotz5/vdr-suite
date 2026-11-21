#include "BasicHttpClient.h"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace {
using Clock = std::chrono::steady_clock;
constexpr auto POLL_INTERVAL = std::chrono::milliseconds(250);

struct SocketCloser {
    void operator()(int* fd) const noexcept {
        if (fd != nullptr) {
            if (*fd >= 0) close(*fd);
            delete fd;
        }
    }
};
using Socket = std::unique_ptr<int, SocketCloser>;
Socket ownSocket(int fd) { return Socket(new int(fd)); }

std::string buildHttpRequest(const std::string& host, int port, const HttpRequest& request)
{
    std::ostringstream stream;
    stream << request.method << " " << request.url << " HTTP/1.0\r\n";
    stream << "Host: " << host << ":" << port << "\r\n";
    stream << "Connection: close\r\n";
    for (const auto& header : request.headers)
        stream << header.first << ": " << header.second << "\r\n";
    if (!request.body.empty())
        stream << "Content-Length: " << request.body.size() << "\r\n";
    stream << "\r\n" << request.body;
    return stream.str();
}

HttpResponse parseHttpResponse(const std::string& raw)
{
    HttpResponse response;
    const std::string separator = "\r\n\r\n";
    const std::size_t bodyStart = raw.find(separator);
    const std::string headerBlock = bodyStart == std::string::npos
        ? raw : raw.substr(0, bodyStart);
    response.body = bodyStart == std::string::npos
        ? std::string() : raw.substr(bodyStart + separator.size());
    std::istringstream headerStream(headerBlock);
    std::string statusLine;
    if (std::getline(headerStream, statusLine)) {
        if (!statusLine.empty() && statusLine.back() == '\r') statusLine.pop_back();
        std::istringstream statusStream(statusLine);
        std::string httpVersion;
        statusStream >> httpVersion >> response.statusCode;
    }
    std::string line;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());
        response.headers[key] = value;
    }
    return response;
}

bool retryableSocketError(int errorNumber)
{
    return errorNumber == EINTR || errorNumber == EAGAIN ||
        errorNumber == EWOULDBLOCK;
}

int noSignalSendFlags()
{
#ifdef MSG_NOSIGNAL
    return MSG_NOSIGNAL;
#else
    return 0;
#endif
}

// All socket operations share one monotonic deadline. A peer that sends a
// byte periodically must not be able to keep a request alive indefinitely.
template <typename Cancel>
void waitForSocket(int fd, short events, Clock::time_point deadline,
                   const Cancel& cancelled)
{
    for (;;) {
        if (cancelled()) throw std::runtime_error("HTTP request cancelled");
        const auto now = Clock::now();
        if (now >= deadline) throw std::runtime_error("HTTP request timed out");
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        const int timeout = static_cast<int>(std::min<long long>(
            POLL_INTERVAL.count(), std::max<long long>(1, remaining.count())));
        pollfd descriptor{};
        descriptor.fd = fd;
        descriptor.events = events;
        const int result = poll(&descriptor, 1, timeout);
        if (result > 0) {
            if (cancelled()) throw std::runtime_error("HTTP request cancelled");
            if (Clock::now() >= deadline) throw std::runtime_error("HTTP request timed out");
            return;
        }
        if (result == 0 || errno == EINTR) continue;
        throw std::runtime_error("poll failed: " + std::string(std::strerror(errno)));
    }
}
}

BasicHttpClient::BasicHttpClient(
    std::string host, int port, IRuntimeLogger* logger,
    IRuntimeMeasurementSink* measurementSink,
    CancellationCheck cancellationCheck,
    std::chrono::milliseconds requestTimeout)
    : host_(std::move(host)), port_(port), logger_(logger),
      measurementSink_(measurementSink),
      cancellationCheck_(std::move(cancellationCheck)),
      requestTimeout_(requestTimeout)
{
    if (requestTimeout_.count() <= 0)
        throw std::invalid_argument("HTTP request timeout must be positive");
}

bool BasicHttpClient::cancellationRequested() const
{
    return cancellationCheck_ && cancellationCheck_();
}

void BasicHttpClient::log(RuntimeLogLevel level, const std::string& message) const
{
    if (logger_ == nullptr) return;
    logger_->write(RuntimeLogEntry{level, "BasicHttpClient", message});
}

void BasicHttpClient::recordMeasurement(const RuntimeMeasurement& measurement) const
{
    if (measurementSink_ == nullptr) return;
    measurementSink_->recordMeasurement(measurement);
}

HttpResponse BasicHttpClient::execute(const HttpRequest& request) const
{
    const auto started = Clock::now();
    const auto deadline = started + requestTimeout_;
    const auto cancelled = [this]() { return cancellationRequested(); };
    if (cancelled()) throw std::runtime_error("HTTP request cancelled");

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    const std::string port = std::to_string(port_);
    const int lookup = getaddrinfo(host_.c_str(), port.c_str(), &hints, &result);
    if (lookup != 0)
        throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(lookup));
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addresses(result, freeaddrinfo);

    Socket socketFd;
    for (addrinfo* current = addresses.get(); current != nullptr; current = current->ai_next) {
        if (cancelled()) throw std::runtime_error("HTTP request cancelled");
        if (Clock::now() >= deadline) throw std::runtime_error("HTTP request timed out");
        const int fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (fd < 0) continue;
        Socket candidate = ownSocket(fd);
        const int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) continue;
        int connected = connect(fd, current->ai_addr, current->ai_addrlen);
        if (connected < 0 && errno == EINTR) connected = -1;
        if (connected < 0 && (errno == EINPROGRESS || errno == EINTR || errno == EALREADY)) {
            waitForSocket(fd, POLLOUT, deadline, cancelled);
            int socketError = 0;
            socklen_t errorLength = sizeof(socketError);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &errorLength) != 0 || socketError != 0)
                continue;
            connected = 0;
        }
        if (connected == 0) {
            socketFd = std::move(candidate);
            break;
        }
    }
    if (!socketFd)
        throw std::runtime_error("connect failed to " + host_ + ":" + port);
    const int fd = *socketFd;
    const std::string rawRequest = buildHttpRequest(host_, port_, request);
    std::size_t bytesSent = 0;
    while (bytesSent < rawRequest.size()) {
        waitForSocket(fd, POLLOUT, deadline, cancelled);
        const ssize_t written = send(fd, rawRequest.data() + bytesSent,
                                     rawRequest.size() - bytesSent, noSignalSendFlags());
        if (written < 0) {
            const int errorNumber = errno;
            if (retryableSocketError(errorNumber)) continue;
            throw std::runtime_error("send failed: " + std::string(std::strerror(errorNumber)));
        }
        if (written == 0) throw std::runtime_error("send failed: connection closed");
        bytesSent += static_cast<std::size_t>(written);
    }

    std::string rawResponse;
    char buffer[4096];
    for (;;) {
        waitForSocket(fd, POLLIN, deadline, cancelled);
        const ssize_t received = recv(fd, buffer, sizeof(buffer), 0);
        if (received < 0) {
            const int errorNumber = errno;
            if (retryableSocketError(errorNumber)) continue;
            throw std::runtime_error("recv failed: " + std::string(std::strerror(errorNumber)));
        }
        if (received == 0) break;
        rawResponse.append(buffer, static_cast<std::size_t>(received));
    }
    socketFd.reset();
    HttpResponse response = parseHttpResponse(rawResponse);
    const auto finished = Clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();
    log(RuntimeLogLevel::Info,
        request.method + " " + request.url + " finished with status " +
        std::to_string(response.statusCode) + " in " + std::to_string(durationMs) +
        " ms, body " + std::to_string(response.body.size()) + " bytes");
    RuntimeMeasurement measurement;
    measurement.component = "BasicHttpClient";
    measurement.operation = request.method + " " + request.url;
    measurement.durationMs = durationMs;
    measurement.statusCode = response.statusCode;
    measurement.sizeBytes = response.body.size();
    recordMeasurement(measurement);
    return response;
}

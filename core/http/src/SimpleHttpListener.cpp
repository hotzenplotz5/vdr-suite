#include "SimpleHttpListener.h"

#include "HttpServerRequest.h"
#include "HttpServerResponse.h"

#include <cerrno>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

constexpr auto DEFAULT_CLIENT_IO_TIMEOUT = std::chrono::seconds(5);
constexpr std::size_t IMAGE_WRITER_THREAD_COUNT = 4;
constexpr std::size_t IMAGE_WRITER_MAX_PENDING = 16;
constexpr std::size_t STREAM_WRITER_THREAD_COUNT = 4;
constexpr std::size_t STREAM_WRITER_MAX_PENDING = 4;
constexpr auto STREAM_FIRST_DATA_TIMEOUT = std::chrono::seconds(5);
constexpr auto STREAM_POLL_INTERVAL = std::chrono::milliseconds(100);

timeval socketTimeoutValue(std::chrono::milliseconds timeout)
{
    if (timeout.count() <= 0) {
        timeout = std::chrono::milliseconds(1);
    }

    const auto totalMicroseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(timeout).count();

    timeval value{};
    value.tv_sec = static_cast<time_t>(totalMicroseconds / 1000000);
    value.tv_usec = static_cast<suseconds_t>(totalMicroseconds % 1000000);
    return value;
}

void configureClientSocketTimeouts(
    int socketFd,
    std::chrono::milliseconds timeout)
{
    const timeval value = socketTimeoutValue(timeout);

    if (setsockopt(
            socketFd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &value,
            sizeof(value)) != 0) {
        std::cerr
            << "failed to configure HTTP client receive timeout: "
            << std::strerror(errno)
            << std::endl;
    }

    if (setsockopt(
            socketFd,
            SOL_SOCKET,
            SO_SNDTIMEO,
            &value,
            sizeof(value)) != 0) {
        std::cerr
            << "failed to configure HTTP client send timeout: "
            << std::strerror(errno)
            << std::endl;
    }
}

int noSignalSendFlags()
{
#ifdef MSG_NOSIGNAL
    return MSG_NOSIGNAL;
#else
    return 0;
#endif
}

std::string lowerAscii(const std::string& value)
{
    std::string lowered;
    lowered.reserve(value.size());

    for (const char character : value) {
        lowered.push_back(
            static_cast<char>(
                std::tolower(
                    static_cast<unsigned char>(character))));
    }

    return lowered;
}

bool hasHeader(
    const HttpServerResponse& response,
    const std::string& headerName)
{
    const std::string wanted = lowerAscii(headerName);

    for (const auto& header : response.headers) {
        if (lowerAscii(header.first) == wanted) {
            return true;
        }
    }

    return false;
}

std::string responseHeaderValue(
    const HttpServerResponse& response,
    const std::string& headerName)
{
    const std::string wanted = lowerAscii(headerName);

    for (const auto& header : response.headers) {
        if (lowerAscii(header.first) == wanted) {
            return header.second;
        }
    }

    return {};
}

bool isImageResponse(const HttpServerResponse& response)
{
    const std::string contentType =
        lowerAscii(responseHeaderValue(response, "Content-Type"));

    return contentType.compare(0, 6, "image/") == 0;
}

std::string serializedHeaderValue(
    const std::string& headerName,
    const std::string& headerValue)
{
    if (lowerAscii(headerName) == "content-type" &&
        lowerAscii(headerValue) == "application/json") {
        return "application/json; charset=utf-8";
    }

    return headerValue;
}

std::string reasonPhrase(int statusCode)
{
    if (statusCode == 200) return "OK";
    if (statusCode == 204) return "No Content";
    if (statusCode == 400) return "Bad Request";
    if (statusCode == 401) return "Unauthorized";
    if (statusCode == 404) return "Not Found";
    if (statusCode == 405) return "Method Not Allowed";
    if (statusCode == 409) return "Conflict";
    if (statusCode == 500) return "Internal Server Error";
    if (statusCode == 503) return "Service Unavailable";
    return "OK";
}

HttpServerRequest parseRequest(const std::string& rawRequest)
{
    HttpServerRequest request;

    const std::size_t headerEnd = rawRequest.find("\r\n\r\n");

    std::string headerBlock = rawRequest;
    if (headerEnd != std::string::npos) {
        headerBlock = rawRequest.substr(0, headerEnd);
        request.body = rawRequest.substr(headerEnd + 4);
    }

    std::istringstream stream(headerBlock);
    std::string requestLine;

    if (!std::getline(stream, requestLine)) {
        return request;
    }

    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    std::string httpVersion;

    requestLineStream
        >> request.method
        >> request.path
        >> httpVersion;

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            break;
        }

        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        while (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }

        request.headers[key] = value;
    }

    return request;
}

std::string serializeResponse(
    const HttpServerResponse& response,
    bool streaming)
{
    std::ostringstream stream;

    stream
        << "HTTP/1.1 "
        << response.statusCode
        << " "
        << reasonPhrase(response.statusCode)
        << "\r\n";

    for (const auto& header : response.headers) {
        stream
            << header.first
            << ": "
            << serializedHeaderValue(header.first, header.second)
            << "\r\n";
    }

    if (!hasHeader(response, "Cache-Control")) {
        stream << "Cache-Control: no-cache\r\n";
    }

    if (!hasHeader(response, "X-Content-Type-Options")) {
        stream << "X-Content-Type-Options: nosniff\r\n";
    }

    if (!streaming) {
        stream
            << "Content-Length: "
            << response.body.size()
            << "\r\n";
    }

    stream << "Connection: close\r\n";
    stream << "\r\n";
    if (!streaming) stream << response.body;

    return stream.str();
}

bool hasCompleteHeaders(const std::string& rawRequest)
{
    return rawRequest.find("\r\n\r\n") != std::string::npos;
}

std::size_t contentLength(const HttpServerRequest& request)
{
    auto iterator = request.headers.find("Content-Length");
    if (iterator == request.headers.end()) {
        return 0;
    }

    char* end = nullptr;
    const unsigned long value =
        std::strtoul(iterator->second.c_str(), &end, 10);

    if (end == iterator->second.c_str()) {
        return 0;
    }

    return static_cast<std::size_t>(value);
}

bool hasCompleteBody(
    const HttpServerRequest& request,
    const std::size_t expectedContentLength)
{
    return request.body.size() >= expectedContentLength;
}

bool writeAll(int socketFd, const char* data, std::size_t size)
{
    std::size_t offset = 0;

    while (offset < size) {
        const ssize_t written = send(
            socketFd,
            data + offset,
            size - offset,
            noSignalSendFlags());

        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return false;
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool writeAll(int socketFd, const std::string& data)
{
    return writeAll(socketFd, data.data(), data.size());
}

std::string imageQueueBusyResponse()
{
    HttpServerResponse response;
    response.statusCode = 503;
    response.headers["Content-Type"] = "text/plain";
    response.headers["Cache-Control"] = "no-store";
    response.body = "image response queue is busy";
    return serializeResponse(response, false);
}

std::string streamQueueBusyResponse()
{
    HttpServerResponse response;
    response.statusCode = 503;
    response.headers["Content-Type"] = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = "{\"error\":{\"code\":\"live_stream_writer_capacity_exhausted\"}}";
    return serializeResponse(response, false);
}

class ImageResponseWriterPool
{
public:
    ImageResponseWriterPool(
        std::size_t workerCount,
        std::size_t maximumPending)
        : maximumPending_(maximumPending)
    {
        workers_.reserve(workerCount);
        for (std::size_t index = 0; index < workerCount; ++index) {
            workers_.emplace_back([this]() { runWorker(); });
        }
    }

    ~ImageResponseWriterPool() { shutdown(); }

    ImageResponseWriterPool(const ImageResponseWriterPool&) = delete;
    ImageResponseWriterPool& operator=(const ImageResponseWriterPool&) = delete;

    bool enqueue(int socketFd, std::string response)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_ || jobs_.size() >= maximumPending_) return false;
        jobs_.push_back(Job{socketFd, std::move(response)});
        condition_.notify_one();
        return true;
    }

    void shutdown()
    {
        std::deque<Job> pending;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) return;
            stopping_ = true;
            pending.swap(jobs_);
        }
        for (const Job& job : pending) {
            if (job.socketFd >= 0) close(job.socketFd);
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
        workers_.clear();
    }

private:
    struct Job
    {
        int socketFd = -1;
        std::string response;
    };

    void runWorker()
    {
        while (true) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]() {
                    return stopping_ || !jobs_.empty();
                });
                if (stopping_ && jobs_.empty()) return;
                job = std::move(jobs_.front());
                jobs_.pop_front();
            }
            writeAll(job.socketFd, job.response);
            close(job.socketFd);
        }
    }

    const std::size_t maximumPending_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<Job> jobs_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
};

class StreamResponseWriterPool
{
public:
    StreamResponseWriterPool(
        std::size_t workerCount,
        std::size_t maximumPending)
        : maximumPending_(maximumPending)
    {
        workers_.reserve(workerCount);
        for (std::size_t index = 0; index < workerCount; ++index) {
            workers_.emplace_back([this]() { runWorker(); });
        }
    }

    ~StreamResponseWriterPool() { shutdown(); }

    StreamResponseWriterPool(const StreamResponseWriterPool&) = delete;
    StreamResponseWriterPool& operator=(const StreamResponseWriterPool&) = delete;

    bool enqueue(int socketFd, std::string headers, std::string streamPath)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_ || jobs_.size() >= maximumPending_) return false;
        jobs_.push_back(Job{socketFd, std::move(headers), std::move(streamPath)});
        condition_.notify_one();
        return true;
    }

    void shutdown()
    {
        std::deque<Job> pending;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) return;
            stopping_ = true;
            pending.swap(jobs_);
        }
        for (const Job& job : pending) {
            if (job.socketFd >= 0) close(job.socketFd);
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
        workers_.clear();
    }

private:
    struct Job
    {
        int socketFd = -1;
        std::string headers;
        std::string streamPath;
    };

    bool stopping() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopping_;
    }

    void stream(Job& job)
    {
        if (!writeAll(job.socketFd, job.headers)) return;

        struct stat status {};
        if (::lstat(job.streamPath.c_str(), &status) != 0 || !S_ISFIFO(status.st_mode))
            return;

        const int input = ::open(
            job.streamPath.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (input < 0) return;

        const auto firstDataDeadline =
            std::chrono::steady_clock::now() + STREAM_FIRST_DATA_TIMEOUT;
        bool sawData = false;
        char buffer[64 * 1024];

        while (!stopping()) {
            pollfd descriptor {};
            descriptor.fd = input;
            descriptor.events = POLLIN;
            const int ready = ::poll(
                &descriptor,
                1,
                static_cast<int>(STREAM_POLL_INTERVAL.count()));
            if (ready < 0) {
                if (errno == EINTR) continue;
                break;
            }

            if (ready > 0 && (descriptor.revents & POLLIN) != 0) {
                const ssize_t received = ::read(input, buffer, sizeof(buffer));
                if (received > 0) {
                    sawData = true;
                    if (!writeAll(
                            job.socketFd,
                            buffer,
                            static_cast<std::size_t>(received))) {
                        break;
                    }
                    continue;
                }
                if (received < 0 &&
                    (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
                    continue;
                }
                if (received == 0 && sawData) break;
            }

            if (ready > 0 &&
                (descriptor.revents & (POLLERR | POLLNVAL)) != 0) {
                break;
            }
            if (ready > 0 && (descriptor.revents & POLLHUP) != 0 && sawData)
                break;

            if (!sawData && std::chrono::steady_clock::now() >= firstDataDeadline)
                break;
        }

        ::close(input);
    }

    void runWorker()
    {
        while (true) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]() {
                    return stopping_ || !jobs_.empty();
                });
                if (stopping_ && jobs_.empty()) return;
                job = std::move(jobs_.front());
                jobs_.pop_front();
            }
            stream(job);
            close(job.socketFd);
        }
    }

    const std::size_t maximumPending_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<Job> jobs_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
};

} // namespace

SimpleHttpListener::SimpleHttpListener(
    std::string host,
    int port,
    IHttpServer& server)
    : SimpleHttpListener(
          std::move(host),
          port,
          server,
          []() { return false; },
          []() {})
{
}

SimpleHttpListener::SimpleHttpListener(
    std::string host,
    int port,
    IHttpServer& server,
    std::function<bool()> shouldStop)
    : SimpleHttpListener(
          std::move(host),
          port,
          server,
          std::move(shouldStop),
          []() {})
{
}

SimpleHttpListener::SimpleHttpListener(
    std::string host,
    int port,
    IHttpServer& server,
    std::function<bool()> shouldStop,
    std::function<void()> onTick)
    : SimpleHttpListener(
          std::move(host),
          port,
          server,
          std::move(shouldStop),
          std::move(onTick),
          DEFAULT_CLIENT_IO_TIMEOUT)
{
}

SimpleHttpListener::SimpleHttpListener(
    std::string host,
    int port,
    IHttpServer& server,
    std::function<bool()> shouldStop,
    std::function<void()> onTick,
    std::chrono::milliseconds clientIoTimeout)
    : host_(std::move(host)),
      port_(port),
      server_(server),
      shouldStop_(std::move(shouldStop)),
      onTick_(std::move(onTick)),
      clientIoTimeout_(clientIoTimeout)
{
}

int SimpleHttpListener::runUntilStopped()
{
    const int listenSocket = createListeningSocket();
    if (listenSocket < 0) return 1;

    ImageResponseWriterPool imageWriters(
        IMAGE_WRITER_THREAD_COUNT,
        IMAGE_WRITER_MAX_PENDING);
    StreamResponseWriterPool streamWriters(
        STREAM_WRITER_THREAD_COUNT,
        STREAM_WRITER_MAX_PENDING);

    std::cout
        << "simple HTTP listener running on "
        << host_
        << ":"
        << port_
        << std::endl;

    while (!shouldStop_()) {
        if (onTick_) onTick_();

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSocket, &readSet);

        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        const int ready = select(
            listenSocket + 1,
            &readSet,
            nullptr,
            nullptr,
            &timeout);

        if (ready < 0) {
            if (errno == EINTR) break;
            std::cerr
                << "select failed: "
                << std::strerror(errno)
                << std::endl;
            continue;
        }
        if (ready == 0) continue;

        const int clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            if (errno == EINTR) break;
            std::cerr
                << "accept failed: "
                << std::strerror(errno)
                << std::endl;
            continue;
        }

        configureClientSocketTimeouts(clientSocket, clientIoTimeout_);

        std::string rawResponse;
        bool imageResponse = false;
        std::string streamBodyPath;

        if (!prepareClientResponse(
                clientSocket,
                rawResponse,
                imageResponse,
                streamBodyPath)) {
            close(clientSocket);
            continue;
        }

        if (!streamBodyPath.empty()) {
            if (streamWriters.enqueue(
                    clientSocket,
                    std::move(rawResponse),
                    std::move(streamBodyPath))) {
                continue;
            }
            writeAll(clientSocket, streamQueueBusyResponse());
            close(clientSocket);
            continue;
        }

        if (imageResponse) {
            if (imageWriters.enqueue(
                    clientSocket,
                    std::move(rawResponse))) {
                continue;
            }
            writeAll(clientSocket, imageQueueBusyResponse());
            close(clientSocket);
            continue;
        }

        writeAll(clientSocket, rawResponse);
        close(clientSocket);
    }

    streamWriters.shutdown();
    imageWriters.shutdown();
    close(listenSocket);
    return 0;
}

int SimpleHttpListener::createListeningSocket() const
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = nullptr;
    const std::string port = std::to_string(port_);

    const int lookup = getaddrinfo(
        host_.c_str(),
        port.c_str(),
        &hints,
        &result);

    if (lookup != 0) {
        std::cerr
            << "getaddrinfo failed: "
            << gai_strerror(lookup)
            << std::endl;
        return -1;
    }

    int listenSocket = -1;

    for (addrinfo* current = result; current != nullptr; current = current->ai_next) {
        listenSocket = socket(
            current->ai_family,
            current->ai_socktype,
            current->ai_protocol);

        if (listenSocket < 0) continue;

        int reuseAddress = 1;
        setsockopt(
            listenSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuseAddress,
            sizeof(reuseAddress));

        if (bind(listenSocket, current->ai_addr, current->ai_addrlen) == 0)
            break;

        close(listenSocket);
        listenSocket = -1;
    }

    freeaddrinfo(result);

    if (listenSocket < 0) {
        std::cerr
            << "failed to bind HTTP listener to "
            << host_
            << ":"
            << port
            << std::endl;
        return -1;
    }

    if (listen(listenSocket, 16) != 0) {
        std::cerr
            << "listen failed: "
            << std::strerror(errno)
            << std::endl;
        close(listenSocket);
        return -1;
    }

    return listenSocket;
}

bool SimpleHttpListener::prepareClientResponse(
    int clientSocket,
    std::string& rawResponse,
    bool& imageResponse,
    std::string& streamBodyPath) const
{
    std::string rawRequest;
    char buffer[4096];

    while (!hasCompleteHeaders(rawRequest)) {
        const ssize_t received = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0);
        if (received <= 0) return false;
        rawRequest.append(buffer, static_cast<std::size_t>(received));
    }

    HttpServerRequest request = parseRequest(rawRequest);
    const std::size_t expectedContentLength = contentLength(request);

    while (!hasCompleteBody(request, expectedContentLength)) {
        const ssize_t received = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0);
        if (received <= 0) return false;
        rawRequest.append(buffer, static_cast<std::size_t>(received));
        request = parseRequest(rawRequest);
    }

    const HttpServerResponse response = server_.handleRequest(request);
    imageResponse = isImageResponse(response);
    streamBodyPath = response.streamBodyPath;
    rawResponse = serializeResponse(response, !streamBodyPath.empty());
    return true;
}

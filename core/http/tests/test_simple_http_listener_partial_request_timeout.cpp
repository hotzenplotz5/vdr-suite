#include "SimpleHttpListener.h"

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace {

class FixedResponseServer : public IHttpServer {
public:
    HttpServerResponse handleRequest(
        const HttpServerRequest&) const override
    {
        handledRequests_.fetch_add(1);

        HttpServerResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "text/plain";
        response.body = "healthy";
        return response;
    }

    int handledRequests() const
    {
        return handledRequests_.load();
    }

private:
    mutable std::atomic<int> handledRequests_{0};
};

class StreamingResponseServer : public IHttpServer {
public:
    explicit StreamingResponseServer(std::string streamPath)
        : streamPath_(std::move(streamPath))
    {
    }

    HttpServerResponse handleRequest(
        const HttpServerRequest&) const override
    {
        HttpServerResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "video/mp4";
        response.headers["Cache-Control"] = "no-store";
        response.streamBodyPath = streamPath_;
        return response;
    }

private:
    std::string streamPath_;
};

int reserveAvailablePort()
{
    const int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        throw std::runtime_error("failed to create port reservation socket");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (bind(
            socketFd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) != 0) {
        close(socketFd);
        throw std::runtime_error("failed to reserve test port");
    }

    socklen_t addressLength = sizeof(address);
    if (getsockname(
            socketFd,
            reinterpret_cast<sockaddr*>(&address),
            &addressLength) != 0) {
        close(socketFd);
        throw std::runtime_error("failed to read reserved test port");
    }

    const int port = ntohs(address.sin_port);
    close(socketFd);
    return port;
}

int connectWithRetry(int port, int receiveBufferBytes = 0)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        const int socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFd < 0) {
            throw std::runtime_error("failed to create test client socket");
        }

        if (receiveBufferBytes > 0 &&
            setsockopt(
                socketFd,
                SOL_SOCKET,
                SO_RCVBUF,
                &receiveBufferBytes,
                sizeof(receiveBufferBytes)) != 0) {
            close(socketFd);
            throw std::runtime_error("failed to configure test receive buffer");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(static_cast<uint16_t>(port));

        if (connect(
                socketFd,
                reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) == 0) {
            return socketFd;
        }

        close(socketFd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    throw std::runtime_error("listener did not accept connections");
}

void sendAll(int socketFd, const std::string& data)
{
    std::size_t offset = 0;

    while (offset < data.size()) {
#ifdef MSG_NOSIGNAL
        const int flags = MSG_NOSIGNAL;
#else
        const int flags = 0;
#endif
        const ssize_t written = send(
            socketFd,
            data.data() + offset,
            data.size() - offset,
            flags);

        if (written <= 0) {
            throw std::runtime_error(
                "failed to send test request: " +
                std::string(std::strerror(errno)));
        }

        offset += static_cast<std::size_t>(written);
    }
}

std::string receiveAll(int socketFd)
{
    timeval timeout{};
    timeout.tv_sec = 5;

    if (setsockopt(
            socketFd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &timeout,
            sizeof(timeout)) != 0) {
        throw std::runtime_error("failed to configure test receive timeout");
    }

    std::string response;
    std::array<char, 64 * 1024> buffer{};

    while (true) {
        const ssize_t received = recv(
            socketFd,
            buffer.data(),
            buffer.size(),
            0);

        if (received == 0) {
            break;
        }

        if (received < 0) {
            throw std::runtime_error(
                "failed to receive response: " +
                std::string(std::strerror(errno)));
        }

        response.append(buffer.data(), static_cast<std::size_t>(received));
    }

    return response;
}

bool writeStreamPayload(const std::string& path, std::size_t bytes)
{
    const int output = ::open(path.c_str(), O_WRONLY | O_CLOEXEC);
    if (output < 0) return false;

    std::array<char, 64 * 1024> payload{};
    payload.fill('x');
    std::size_t remaining = bytes;

    while (remaining > 0) {
        const std::size_t requested = std::min(remaining, payload.size());
        const ssize_t written = ::write(output, payload.data(), requested);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) {
            close(output);
            return false;
        }
        remaining -= static_cast<std::size_t>(written);
    }

    return close(output) == 0;
}

void testPartialRequestTimeout()
{
    const int port = reserveAvailablePort();
    std::atomic<bool> stopRequested(false);
    FixedResponseServer server;

    SimpleHttpListener listener(
        "127.0.0.1",
        port,
        server,
        [&stopRequested]() {
            return stopRequested.load();
        },
        []() {},
        std::chrono::milliseconds(150));

    std::thread listenerThread([&listener]() {
        assert(listener.runUntilStopped() == 0);
    });

    const int stalledClient = connectWithRetry(port);
    sendAll(
        stalledClient,
        "POST /api/recordings/actions/execute HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 100\r\n"
        "Connection: close\r\n"
        "\r\n"
        "{");

    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    const int healthyClient = connectWithRetry(port);
    sendAll(
        healthyClient,
        "GET /health HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
        "\r\n");

    const std::string response = receiveAll(healthyClient);

    assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(response.find("healthy") != std::string::npos);
    assert(server.handledRequests() == 1);

    close(healthyClient);
    close(stalledClient);

    stopRequested.store(true);
    listenerThread.join();
}

void testStreamingBackpressureSurvivesSocketSendTimeout()
{
    constexpr std::size_t PayloadBytes = 16U * 1024U * 1024U;
    const std::string streamPath =
        "/tmp/vdr-suite-http-stream-backpressure-" +
        std::to_string(static_cast<long long>(::getpid())) + ".fifo";
    ::unlink(streamPath.c_str());
    assert(::mkfifo(streamPath.c_str(), 0600) == 0);

    const int port = reserveAvailablePort();
    std::atomic<bool> stopRequested(false);
    StreamingResponseServer server(streamPath);
    SimpleHttpListener listener(
        "127.0.0.1",
        port,
        server,
        [&stopRequested]() {
            return stopRequested.load();
        },
        []() {},
        std::chrono::milliseconds(100));

    std::thread listenerThread([&listener]() {
        assert(listener.runUntilStopped() == 0);
    });

    const int client = connectWithRetry(port, 4096);
    sendAll(
        client,
        "GET /stream HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
        "\r\n");

    std::atomic<bool> producerSucceeded(false);
    std::thread producer([&]() {
        producerSucceeded.store(writeStreamPayload(streamPath, PayloadBytes));
    });

    // Deliberately exceed the listener's socket send timeout while the client
    // does not consume the response. Continuous media must treat this as
    // backpressure, not as a disconnect.
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    const std::string response = receiveAll(client);
    producer.join();

    assert(producerSucceeded.load());
    assert(response.find("HTTP/1.1 200 OK") == 0);
    const std::size_t headerEnd = response.find("\r\n\r\n");
    assert(headerEnd != std::string::npos);
    const std::size_t bodyOffset = headerEnd + 4;
    assert(response.size() - bodyOffset == PayloadBytes);
    assert(std::all_of(
        response.begin() + static_cast<std::string::difference_type>(bodyOffset),
        response.end(),
        [](char value) { return value == 'x'; }));

    close(client);
    stopRequested.store(true);
    listenerThread.join();
    ::unlink(streamPath.c_str());
}

} // namespace

int main()
{
    ::signal(SIGPIPE, SIG_IGN);
    testPartialRequestTimeout();
    testStreamingBackpressureSurvivesSocketSendTimeout();
    return 0;
}

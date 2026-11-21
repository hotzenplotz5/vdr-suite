#include "SimpleHttpListener.h"

#include <arpa/inet.h>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

class ImageAndApiServer : public IHttpServer
{
public:
    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override
    {
        handledRequests_.fetch_add(1);

        HttpServerResponse response;

        if (request.path == "/slow-image") {
            imageRequests_.fetch_add(1);
            response.statusCode = 200;
            response.headers["Content-Type"] = "image/jpeg";
            response.body.assign(16U * 1024U * 1024U, 'x');
            return response;
        }

        apiRequests_.fetch_add(1);
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"genrePage\":true}";
        return response;
    }

    int handledRequests() const { return handledRequests_.load(); }
    int imageRequests() const { return imageRequests_.load(); }
    int apiRequests() const { return apiRequests_.load(); }

private:
    mutable std::atomic<int> handledRequests_{0};
    mutable std::atomic<int> imageRequests_{0};
    mutable std::atomic<int> apiRequests_{0};
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

int connectWithRetry(int port)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        const int socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFd < 0) {
            throw std::runtime_error("failed to create test client socket");
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

std::string receiveAllWithTimeout(
    int socketFd,
    std::chrono::milliseconds timeout)
{
    timeval value{};
    value.tv_sec = static_cast<time_t>(timeout.count() / 1000);
    value.tv_usec = static_cast<suseconds_t>(
        (timeout.count() % 1000) * 1000);

    if (setsockopt(
            socketFd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &value,
            sizeof(value)) != 0) {
        throw std::runtime_error("failed to configure test receive timeout");
    }

    std::string response;
    char buffer[1024];

    while (true) {
        const ssize_t received = recv(
            socketFd,
            buffer,
            sizeof(buffer),
            0);

        if (received == 0) {
            break;
        }

        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            throw std::runtime_error(
                "failed to receive response: " +
                std::string(std::strerror(errno)));
        }

        response.append(buffer, static_cast<std::size_t>(received));
    }

    return response;
}

void waitForImageRequest(const ImageAndApiServer& server)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (server.imageRequests() == 1) {
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    throw std::runtime_error("slow image request was not handled");
}

} // namespace

int main()
{
    const int port = reserveAvailablePort();
    std::atomic<bool> stopRequested(false);
    ImageAndApiServer server;

    SimpleHttpListener listener(
        "127.0.0.1",
        port,
        server,
        [&stopRequested]() {
            return stopRequested.load();
        },
        []() {},
        std::chrono::milliseconds(2500));

    std::thread listenerThread([&listener]() {
        assert(listener.runUntilStopped() == 0);
    });

    const int slowClient = connectWithRetry(port);

    int receiveBufferBytes = 1024;
    assert(setsockopt(
        slowClient,
        SOL_SOCKET,
        SO_RCVBUF,
        &receiveBufferBytes,
        sizeof(receiveBufferBytes)) == 0);

    sendAll(
        slowClient,
        "GET /slow-image HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
        "\r\n");

    waitForImageRequest(server);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto startedAt = std::chrono::steady_clock::now();
    const int apiClient = connectWithRetry(port);

    sendAll(
        apiClient,
        "GET /api/metadata/genres/epg?backend=default&genre=drama "
        "HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
        "\r\n");

    const std::string apiResponse = receiveAllWithTimeout(
        apiClient,
        std::chrono::milliseconds(1200));

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startedAt);

    assert(apiResponse.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(apiResponse.find("{\"genrePage\":true}") != std::string::npos);
    assert(elapsed < std::chrono::milliseconds(1200));
    assert(server.handledRequests() == 2);
    assert(server.apiRequests() == 1);

    close(apiClient);
    close(slowClient);

    stopRequested.store(true);
    listenerThread.join();

    return 0;
}

#include "SimpleHttpListener.h"

#include <arpa/inet.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
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

std::string receiveAll(int socketFd)
{
    timeval timeout{};
    timeout.tv_sec = 2;

    if (setsockopt(
            socketFd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &timeout,
            sizeof(timeout)) != 0) {
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
            throw std::runtime_error(
                "failed to receive healthy response: " +
                std::string(std::strerror(errno)));
        }

        response.append(buffer, static_cast<std::size_t>(received));
    }

    return response;
}

} // namespace

int main()
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

    return 0;
}

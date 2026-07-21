#include "SimpleHttpListener.h"

#include <arpa/inet.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{

using Clock = std::chrono::steady_clock;

class HeadOfLineServer final : public IHttpServer
{
public:
    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override
    {
        HttpServerResponse response;
        response.statusCode = 200;

        if (request.path == "/slow-image")
        {
            response.headers["Content-Type"] = "image/jpeg";
            response.body.assign(16 * 1024 * 1024, 'x');
            slowImageHandled_.store(true);
            return response;
        }

        response.headers["Content-Type"] = "application/json";
        response.body = "{\"recordingFolder\":true,\"path\":\"Control\"}";
        return response;
    }

    bool slowImageHandled() const
    {
        return slowImageHandled_.load();
    }

private:
    mutable std::atomic<bool> slowImageHandled_{false};
};

int reserveLoopbackPort()
{
    const int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(socketFd >= 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    assert(bind(
        socketFd,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)) == 0);

    socklen_t addressLength = sizeof(address);
    assert(getsockname(
        socketFd,
        reinterpret_cast<sockaddr*>(&address),
        &addressLength) == 0);

    const int port = ntohs(address.sin_port);
    close(socketFd);
    return port;
}

int connectWithRetry(int port)
{
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        const int socketFd = socket(AF_INET, SOCK_STREAM, 0);
        assert(socketFd >= 0);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(static_cast<unsigned short>(port));

        if (connect(
                socketFd,
                reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) == 0)
        {
            return socketFd;
        }

        close(socketFd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return -1;
}

void configureReceiveTimeout(
    int socketFd,
    std::chrono::milliseconds timeout)
{
    timeval value{};
    value.tv_sec = static_cast<time_t>(timeout.count() / 1000);
    value.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);

    assert(setsockopt(
        socketFd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &value,
        sizeof(value)) == 0);
}

void sendAll(
    int socketFd,
    const std::string& request)
{
    std::size_t offset = 0;

    while (offset < request.size())
    {
        const ssize_t sent = send(
            socketFd,
            request.data() + offset,
            request.size() - offset,
            0);

        assert(sent > 0);
        offset += static_cast<std::size_t>(sent);
    }
}

std::string readAll(int socketFd)
{
    std::string response;
    char buffer[4096];

    while (true)
    {
        const ssize_t received = recv(
            socketFd,
            buffer,
            sizeof(buffer),
            0);

        if (received <= 0)
        {
            break;
        }

        response.append(buffer, static_cast<std::size_t>(received));
    }

    return response;
}

void waitUntilSlowImageIsPrepared(
    const HeadOfLineServer& server)
{
    for (int attempt = 0; attempt < 200; ++attempt)
    {
        if (server.slowImageHandled())
        {
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    assert(false);
}

}

int main()
{
    const int port = reserveLoopbackPort();
    HeadOfLineServer server;
    std::atomic<bool> stopRequested{false};

    SimpleHttpListener listener(
        "127.0.0.1",
        port,
        server,
        [&stopRequested]() {
            return stopRequested.load();
        },
        []() {},
        std::chrono::milliseconds(1500));

    int listenerResult = -1;
    std::thread listenerThread([&listener, &listenerResult]() {
        listenerResult = listener.runUntilStopped();
    });

    const int slowClient = connectWithRetry(port);
    assert(slowClient >= 0);
    sendAll(
        slowClient,
        "GET /slow-image HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n\r\n");

    waitUntilSlowImageIsPrepared(server);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    const Clock::time_point startedAt = Clock::now();
    const int folderClient = connectWithRetry(port);
    assert(folderClient >= 0);
    configureReceiveTimeout(
        folderClient,
        std::chrono::milliseconds(2000));

    sendAll(
        folderClient,
        "GET /api/vdr/recordings/folder?backend=default&path=Control "
        "HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n\r\n");

    const std::string folderResponse = readAll(folderClient);
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - startedAt);

    close(folderClient);

    assert(folderResponse.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(folderResponse.find("\"recordingFolder\":true") != std::string::npos);
    assert(folderResponse.find("\"path\":\"Control\"") != std::string::npos);
    assert(elapsed < std::chrono::milliseconds(750));

    close(slowClient);
    stopRequested.store(true);

    const int wakeClient = connectWithRetry(port);
    if (wakeClient >= 0)
    {
        close(wakeClient);
    }

    listenerThread.join();
    assert(listenerResult == 0);

    return 0;
}

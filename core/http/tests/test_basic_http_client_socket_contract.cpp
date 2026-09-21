#include "BasicHttpClient.h"

#include <arpa/inet.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{
int createListeningSocket(int& port)
{
    const int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(socketFd >= 0);

    int reuseAddress = 1;
    assert(setsockopt(
        socketFd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuseAddress,
        sizeof(reuseAddress)) == 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    assert(bind(
        socketFd,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0);
    assert(listen(socketFd, 1) == 0);

    socklen_t addressLength = sizeof(address);
    assert(getsockname(
        socketFd,
        reinterpret_cast<sockaddr*>(&address),
        &addressLength) == 0);

    port = ntohs(address.sin_port);
    return socketFd;
}
}

int main()
{
    {
        BasicHttpClient client("127.0.0.1", 1);

        HttpRequest request;
        request.method = "GET";
        request.url = "/api/info.json";

        bool failedAsExpected = false;

        try
        {
            client.execute(request);
        }
        catch (const std::runtime_error& error)
        {
            const std::string message = error.what();
            failedAsExpected =
                message.find("connect failed to 127.0.0.1:1") != std::string::npos;
        }

        assert(failedAsExpected);
    }

    int port = 0;
    const int listenSocket = createListeningSocket(port);
    std::atomic<bool> cancellationRequested(false);
    std::atomic<bool> requestAccepted(false);

    std::thread server([&]() {
        const int clientSocket = accept(listenSocket, nullptr, nullptr);
        assert(clientSocket >= 0);
        requestAccepted.store(true);

        char buffer[1024];
        while (recv(clientSocket, buffer, sizeof(buffer), 0) > 0)
        {
        }

        close(clientSocket);
        close(listenSocket);
    });

    BasicHttpClient cancellableClient(
        "127.0.0.1",
        port,
        nullptr,
        nullptr,
        [&]() {
            return cancellationRequested.load();
        });

    HttpRequest request;
    request.method = "GET";
    request.url = "/slow.json";

    bool cancelledAsExpected = false;
    const auto started = std::chrono::steady_clock::now();

    std::thread client([&]() {
        try
        {
            cancellableClient.execute(request);
        }
        catch (const std::runtime_error& error)
        {
            cancelledAsExpected =
                std::string(error.what()) == "HTTP request cancelled";
        }
    });

    for (int attempt = 0;
         attempt < 50 && !requestAccepted.load();
         ++attempt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    assert(requestAccepted.load());
    cancellationRequested.store(true);

    client.join();
    server.join();

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count();

    assert(cancelledAsExpected);
    assert(elapsed < 1500);

    return 0;
}

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

using namespace std::chrono_literals;

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    assert(listener >= 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    assert(bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
    assert(listen(listener, 2) == 0);
    socklen_t length = sizeof(address);
    assert(getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) == 0);
    const int port = ntohs(address.sin_port);
    HttpRequest request;
    request.method = "GET";
    request.url = "/info.json";
    std::thread server([&]() {
        for (int i = 0; i < 2; ++i) {
            const int fd = accept(listener, nullptr, nullptr);
            assert(fd >= 0);
            char buffer[1024];
            assert(recv(fd, buffer, sizeof(buffer), 0) > 0);
            if (i == 1) {
                const std::string response = "HTTP/1.0 200 OK\r\nContent-Length: 2\r\n\r\n{}";
                assert(send(fd, response.data(), response.size(), 0) == static_cast<ssize_t>(response.size()));
            } else {
                std::this_thread::sleep_for(350ms);
            }
            close(fd);
        }
        close(listener);
    });
    BasicHttpClient client("127.0.0.1", port, nullptr, nullptr, {}, 100ms);
    const auto started = std::chrono::steady_clock::now();
    bool timedOut = false;
    try { client.execute(request); }
    catch (const std::runtime_error& error) {
        timedOut = std::string(error.what()) == "HTTP request timed out";
    }
    assert(timedOut);
    assert(std::chrono::steady_clock::now() - started < 1000ms);
    BasicHttpClient normal("127.0.0.1", port);
    const auto response = normal.execute(request);
    assert(response.statusCode == 200);
    assert(response.body == "{}");
    server.join();
    bool rejected = false;
    try { BasicHttpClient invalid("127.0.0.1", port, nullptr, nullptr, {}, 0ms); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    return 0;
}

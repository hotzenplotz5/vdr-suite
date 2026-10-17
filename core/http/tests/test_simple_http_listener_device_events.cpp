#include "SimpleHttpListener.h"
#include "DeviceLiveEventHub.h"

#include <arpa/inet.h>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {
class FakeAdmittedEventServer final : public IHttpServer {
public:
    explicit FakeAdmittedEventServer(DeviceLiveEventHub& hub) : hub_(hub) {}
    HttpServerResponse handleRequest(const HttpServerRequest& request) const override {
        HttpServerResponse reply;
        reply.headers["Cache-Control"] = "no-store";
        if (request.path == "/device-events") {
            reply.deviceEventSubscription = hub_.subscribe("home", {"timers"});
            reply.headers["Content-Type"] = "text/event-stream";
        } else {
            reply.headers["Content-Type"] = "text/plain";
            reply.body = "normal-api-response";
        }
        return reply;
    }
private:
    DeviceLiveEventHub& hub_;
};

int portAvailable() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket failed");
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))) {
        ::close(fd);
        throw std::runtime_error("bind failed");
    }
    socklen_t length = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &length)) {
        ::close(fd);
        throw std::runtime_error("getsockname failed");
    }
    int port = ntohs(addr.sin_port);
    ::close(fd);
    return port;
}

int connectPort(int port) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(fd >= 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(static_cast<uint16_t>(port));
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr),
                      sizeof(addr)) == 0)
            return fd;
        ::close(fd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    throw std::runtime_error("listener not ready");
}

void request(int fd, const std::string& target) {
    const std::string line = "GET " + target +
        " HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    assert(::send(fd, line.data(), line.size(), 0) ==
           static_cast<ssize_t>(line.size()));
}

std::string readUntil(int fd, const std::string& needle,
                      std::chrono::milliseconds budget) {
    timeval timeout{};
    timeout.tv_usec = 100000;
    assert(::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                        &timeout, sizeof(timeout)) == 0);
    std::string buffer;
    const auto deadline = std::chrono::steady_clock::now() + budget;
    char chunk[8192];
    while (std::chrono::steady_clock::now() < deadline) {
        const ssize_t size = ::recv(fd, chunk, sizeof(chunk), 0);
        if (size > 0) {
            buffer.append(chunk, static_cast<std::size_t>(size));
            if (buffer.find(needle) != std::string::npos) return buffer;
        } else if (size == 0) {
            break;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK &&
                   errno != EINTR) {
            break;
        }
    }
    return buffer;
}
}

int main() {
    DeviceLiveEventHub hub;
    FakeAdmittedEventServer server(hub);
    const int port = portAvailable();
    std::atomic<bool> stopping(false);
    SimpleHttpListener listener("127.0.0.1", port, server,
        [&stopping]() { return stopping.load(); }, []() {},
        std::chrono::milliseconds(1000));
    std::thread worker([&listener]() {
        assert(listener.runUntilStopped() == 0);
    });
    int stream = connectPort(port);
    request(stream, "/device-events");
    const auto hello = readUntil(stream, "event: hello",
        std::chrono::milliseconds(1500));
    assert(hello.find("HTTP/1.1 200 OK") == 0);
    assert(hello.find("text/event-stream") != std::string::npos);
    assert(hello.find("Content-Length") == std::string::npos);

    hub.publish(LiveUpdateEvent(17, 19, {"timers", "events"}, "home"));
    const auto update = readUntil(stream, "\"changedDomains\":[\"timers\"]",
        std::chrono::milliseconds(1500));
    assert(update.find("\"changedDomains\":[\"timers\"]") !=
           std::string::npos);
    int normal = connectPort(port);
    request(normal, "/ping");
    const auto response = readUntil(normal, "normal-api-response",
        std::chrono::milliseconds(1500));
    assert(response.find("HTTP/1.1 200 OK") == 0);
    ::close(normal);

    hub.disconnectAll();
    stopping.store(true);
    worker.join();
    ::close(stream);
    return 0;
}

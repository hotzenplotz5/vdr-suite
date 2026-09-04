#include "SuiteBridgeSvdrpTransport.h"

#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

using namespace vdrsuite::agent;

namespace
{
void sendAll(int fd, const std::string& text)
{
    std::size_t offset = 0;
    while (offset < text.size())
    {
        const ssize_t sent = send(
            fd,
            text.data() + offset,
            text.size() - offset,
            0);
        assert(sent > 0);
        offset += static_cast<std::size_t>(sent);
    }
}

class Server
{
public:
    explicit Server(std::string reply)
        : reply_(std::move(reply))
    {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        assert(fd_ >= 0);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        assert(bind(
            fd_,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == 0);

        socklen_t length = sizeof(address);
        assert(getsockname(
            fd_,
            reinterpret_cast<sockaddr*>(&address),
            &length) == 0);
        port_ = ntohs(address.sin_port);
        assert(listen(fd_, 1) == 0);

        worker_ = std::thread([this]() {
            const int client = accept(fd_, nullptr, nullptr);
            assert(client >= 0);
            sendAll(client, "220 local-vdr ready\r\n");

            char buffer[512];
            while (request_.find('\n') == std::string::npos)
            {
                const ssize_t received = recv(
                    client,
                    buffer,
                    sizeof(buffer),
                    0);
                assert(received > 0);
                request_.append(
                    buffer,
                    static_cast<std::size_t>(received));
            }

            sendAll(client, reply_);
            shutdown(client, SHUT_WR);
            close(client);
        });
    }

    ~Server()
    {
        if (worker_.joinable())
        {
            worker_.join();
        }
        close(fd_);
    }

    int port() const noexcept { return port_; }

    void wait()
    {
        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    const std::string& request() const noexcept { return request_; }

private:
    int fd_ = -1;
    int port_ = 0;
    std::thread worker_;
    std::string request_;
    std::string reply_;
};

SuiteBridgeSvdrpTransportConfig configFor(const Server& server)
{
    SuiteBridgeSvdrpTransportConfig config;
    config.port = server.port();
    config.connectTimeout = std::chrono::milliseconds(300);
    config.ioTimeout = std::chrono::milliseconds(300);
    config.operationTimeout = std::chrono::milliseconds(1000);
    return config;
}
}

int main()
{
    const std::string key = "c94d0eb9958a85079f81f059a436003c";

    {
        Server server(
            "250 {\"schema\":1,\"found\":true,"
            "\"reason\":\"none\",\"recordingIdentitySchema\":1,"
            "\"recordingKey\":\"" + key + "\","
            "\"state\":\"none\",\"framesPerSecond\":25,"
            "\"isPesRecording\":false,\"inUseFlags\":0,"
            "\"marksFilePresent\":false,\"sequenceCount\":0,"
            "\"marksRevision\":\"0123456789abcdef0123456789abcdef\","
            "\"marks\":[]}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeRecordingMarksCommandReply reply =
            transport.requestRecordingMarks(key);

        server.wait();
        assert(reply.transportSucceeded);
        assert(reply.replyCode == 250);
        assert(reply.payload.find("\"marks\":[]") != std::string::npos);
        assert(server.request() ==
            "PLUG suitebridge RMARKS " + key + "\r\n");
    }

    {
        SuiteBridgeSvdrpTransport transport;
        assert(!transport.requestRecordingMarks(
            "C94D0EB9958A85079F81F059A436003C").transportSucceeded);
        assert(!transport.requestRecordingMarks(
            "/srv/vdr/video/test.rec").transportSucceeded);
        assert(!transport.requestRecordingMarks(
            "%2fsrv%2fvdr").transportSucceeded);
    }

    {
        Server server("451 Recording marks payload unavailable\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeRecordingMarksCommandReply failed =
            transport.requestRecordingMarks(key);

        server.wait();
        assert(!failed.transportSucceeded);
        assert(failed.replyCode == 451);
        assert(failed.payload == "Recording marks payload unavailable");
    }

    return 0;
}

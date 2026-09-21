#include "SuiteBridgeSvdrpTransport.h"

#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

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
                request_.append(buffer, static_cast<std::size_t>(received));
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

    int port() const { return port_; }
    void wait()
    {
        if (worker_.joinable())
        {
            worker_.join();
        }
    }
    const std::string& request() const { return request_; }

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
    static_assert(
        SuiteBridgeSvdrpTransport::MaximumReplyBytes >= 65536,
        "recording metadata replies must permit the expanded bounded payload");

    const std::string key = "c94d0eb9958a85079f81f059a436003c";

    {
        Server server(
            "250 {\"schema\":1,\"found\":true,"
            "\"recordingKey\":\"" + key + "\","
            "\"provider\":\"tvscraper\","
            "\"mediaType\":\"movie\","
            "\"people\":[],\"images\":[]}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeRecordingMetadataCommandReply reply =
            transport.requestRecordingMetadata(key);

        server.wait();
        assert(reply.transportSucceeded);
        assert(reply.replyCode == 250);
        assert(reply.payload.find("\"found\":true") != std::string::npos);
        assert(server.request() ==
            "PLUG suitebridge RMETA " + key + "\r\n");
    }

    {
        const std::string largePayload =
            "{\"schema\":1,\"padding\":\"" +
            std::string(12000, 'x') + "\"}";
        assert(largePayload.size() > 8192);

        Server server("250 " + largePayload + "\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeRecordingMetadataCommandReply reply =
            transport.requestRecordingMetadata(key);

        server.wait();
        assert(reply.transportSucceeded);
        assert(reply.replyCode == 250);
        assert(reply.payload == largePayload);
    }

    {
        SuiteBridgeSvdrpTransport transport;
        assert(!transport.requestRecordingMetadata(
            "C94D0EB9958A85079F81F059A436003C").transportSucceeded);
        assert(!transport.requestRecordingMetadata(
            "/srv/vdr/video/test.rec").transportSucceeded);
        assert(!transport.requestRecordingMetadata(
            "%2fsrv%2fvdr").transportSucceeded);
    }

    {
        Server server("451 Recording metadata provider unavailable\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeRecordingMetadataCommandReply failed =
            transport.requestRecordingMetadata(key);

        server.wait();
        assert(!failed.transportSucceeded);
        assert(failed.replyCode == 451);
        assert(failed.payload == "Recording metadata provider unavailable");
    }

    return 0;
}

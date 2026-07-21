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
    {
        Server server(
            "250 {\"schema\":1,\"found\":true,"
            "\"provider\":\"tvscraper\","
            "\"mediaType\":\"series\","
            "\"title\":\"Testserie\","
            "\"people\":[],\"images\":[]}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeMetadataCommandReply reply = transport.requestMetadata(
            "S19.2E-1-1011-11100",
            "12345");

        server.wait();
        assert(reply.transportSucceeded);
        assert(reply.replyCode == 250);
        assert(reply.payload.find("\"found\":true") != std::string::npos);
        assert(reply.payload.find("\"title\":\"Testserie\"") != std::string::npos);
        assert(server.request() ==
            "PLUG suitebridge META S19.2E-1-1011-11100 12345\r\n");

        const SuiteBridgeMetadataCommandReply cached =
            transport.requestMetadata(
                "S19.2E-1-1011-11100",
                "12345");
        assert(cached.transportSucceeded);
        assert(cached.replyCode == 250);
        assert(cached.payload == reply.payload);
        assert(server.request() ==
            "PLUG suitebridge META S19.2E-1-1011-11100 12345\r\n");

        const SuiteBridgeMetadataCommandReply rejected = transport.requestMetadata(
            "bad channel",
            "12345");
        assert(!rejected.transportSucceeded);

        const SuiteBridgeMetadataCommandReply rejectedEvent = transport.requestMetadata(
            "S19.2E-1-1011-11100",
            "bad event");
        assert(!rejectedEvent.transportSucceeded);
    }

    {
        Server server("451 Metadata payload exceeds contract capacity\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeMetadataCommandReply failed = transport.requestMetadata(
            "S19.2E-1-1011-11100",
            "12345");

        server.wait();
        assert(!failed.transportSucceeded);
        assert(failed.replyCode == 451);
        assert(failed.payload == "Metadata payload exceeds contract capacity");
    }

    return 0;
}

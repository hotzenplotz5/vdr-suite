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
            "250 {\"schemaVersion\":1,\"provider\":\"osdteletext\","
            "\"capability\":\"broadcast.teletext.page\","
            "\"available\":true,\"rows\":25,\"columns\":40}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeTeletextCommandReply reply =
            transport.discoverTeletext();

        server.wait();
        assert(reply.transportSucceeded);
        assert(reply.replyCode == 250);
        assert(reply.payload.find("\"available\":true") != std::string::npos);
        assert(server.request() == "PLUG suitebridge TTXC 1\r\n");
    }

    {
        Server server(
            "250 {\"schemaVersion\":1,\"result\":\"ok\","
            "\"channel\":\"C-1-1051-10301\",\"page\":100,"
            "\"source\":\"live\",\"rows\":25,\"columns\":40}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        SuiteBridgeTeletextPageRequest request;
        request.channelId = "C-1-1051-10301";
        request.pageNumber = 100;
        request.automaticSubpage = true;

        const SuiteBridgeTeletextCommandReply reply =
            transport.requestTeletextPage(request);

        server.wait();
        assert(reply.transportSucceeded);
        assert(reply.replyCode == 250);
        assert(reply.payload.find("\"result\":\"ok\"") != std::string::npos);
        assert(server.request() ==
            "PLUG suitebridge TTXP 1 C-1-1051-10301 100 auto\r\n");
    }

    {
        Server server("250 {\"schemaVersion\":1,\"result\":\"ok\"}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        SuiteBridgeTeletextPageRequest request;
        request.channelId = "C-1-1051-10301";
        request.pageNumber = 777;
        request.automaticSubpage = false;
        request.subpageCode = 42;

        const SuiteBridgeTeletextCommandReply reply =
            transport.requestTeletextPage(request);

        server.wait();
        assert(reply.transportSucceeded);
        assert(server.request() ==
            "PLUG suitebridge TTXP 1 C-1-1051-10301 777 42\r\n");
    }

    {
        SuiteBridgeSvdrpTransport transport;

        SuiteBridgeTeletextPageRequest request;
        request.channelId = "bad channel";
        request.pageNumber = 100;
        assert(!transport.requestTeletextPage(request).transportSucceeded);

        request.channelId = "C-1-1051-10301";
        request.pageNumber = 99;
        assert(!transport.requestTeletextPage(request).transportSucceeded);

        request.pageNumber = 900;
        assert(!transport.requestTeletextPage(request).transportSucceeded);

        request.pageNumber = 100;
        request.automaticSubpage = false;
        request.subpageCode = 0xffffU;
        assert(!transport.requestTeletextPage(request).transportSucceeded);
    }

    {
        Server server("550 {\"schemaVersion\":1,\"result\":\"rejected\"}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeTeletextCommandReply failed =
            transport.discoverTeletext();

        server.wait();
        assert(!failed.transportSucceeded);
        assert(failed.replyCode == 550);
        assert(failed.payload.find("\"result\":\"rejected\"") != std::string::npos);
    }

    return 0;
}

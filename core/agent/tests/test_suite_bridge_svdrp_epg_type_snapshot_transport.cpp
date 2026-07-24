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
                request_.append(buffer, static_cast<std::size_t>(received));
            }

            sendAll(client, reply_);
            shutdown(client, SHUT_WR);
            close(client);
        });
    }

    ~Server()
    {
        if (worker_.joinable()) worker_.join();
        close(fd_);
    }

    int port() const { return port_; }
    void wait()
    {
        if (worker_.joinable()) worker_.join();
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
            "250 1|64|64|0|S19.2E-1-1011-11100,12345,100,200,S;"
            "C-1-1079-10351,23456,110,230,M\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeEpgTypeSnapshotTransportPage page =
            transport.requestEpgTypeSnapshot(100, 300, 0, 64);

        server.wait();
        assert(page.transportSucceeded);
        assert(page.payloadValid);
        assert(page.replyCode == 250);
        assert(page.nextOffset == 64);
        assert(page.scanned == 64);
        assert(!page.done);
        assert(page.items.size() == 2);
        assert(page.items[0].channelId == "S19.2E-1-1011-11100");
        assert(page.items[0].eventId == "12345");
        assert(page.items[0].mediaType == EpgScraperMediaType::Series);
        assert(page.items[1].mediaType == EpgScraperMediaType::Movie);
        assert(server.request() ==
            "PLUG suitebridge ETYPES 100 300 0 64\r\n");
    }

    {
        Server server("250 1|12|0|1|\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeEpgTypeSnapshotTransportPage page =
            transport.requestEpgTypeSnapshot(100, 300, 12, 64);
        server.wait();
        assert(page.transportSucceeded);
        assert(page.payloadValid);
        assert(page.done);
        assert(page.scanned == 0);
        assert(page.items.empty());
    }

    {
        Server server("250 1|63|64|0|\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeEpgTypeSnapshotTransportPage page =
            transport.requestEpgTypeSnapshot(100, 300, 0, 64);
        server.wait();
        assert(page.transportSucceeded);
        assert(!page.payloadValid);
    }

    return 0;
}

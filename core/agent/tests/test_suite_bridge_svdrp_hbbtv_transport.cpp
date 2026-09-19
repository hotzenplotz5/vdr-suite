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

std::string runtimeReply(
    const char* operation,
    const char* result,
    int resultCode,
    const char* state,
    int stateCode)
{
    return
        std::string("250 {\"schemaVersion\":1,") +
        "\"provider\":\"vdr-plugin-web\"," +
        "\"providerSchemaVersion\":1," +
        "\"capability\":\"broadcast.hbbtv.runtime\"," +
        "\"operation\":\"" + operation + "\"," +
        "\"result\":\"" + result + "\"," +
        "\"resultCode\":" + std::to_string(resultCode) + "," +
        "\"state\":\"" + state + "\"," +
        "\"stateCode\":" + std::to_string(stateCode) + "," +
        "\"sessionId\":\"session-a\"," +
        "\"channel\":\"C-1-1051-10301\"," +
        "\"applicationId\":1," +
        "\"descriptorRevision\":7}\r\n";
}

}

int main()
{
    {
        Server server(
            "250 {\"schemaVersion\":1,\"provider\":\"vdr-plugin-web\","
            "\"providerSchemaVersion\":1,"
            "\"capability\":\"broadcast.hbbtv.discovery\","
            "\"result\":\"ok\",\"resultCode\":0,"
            "\"channel\":\"C-1-1051-10301\","
            "\"receiverActive\":true,\"revision\":7,"
            "\"observedAt\":1789670000,\"applications\":[]}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeHbbtvCommandReply reply =
            transport.discoverHbbtv("C-1-1051-10301");

        server.wait();
        assert(reply.transportSucceeded);
        assert(reply.replyCode == 250);
        assert(reply.payload.find(
            "\"capability\":\"broadcast.hbbtv.discovery\"") !=
            std::string::npos);
        assert(server.request() ==
            "PLUG suitebridge HBBAPPS 1 C-1-1051-10301\r\n");
    }

    {
        Server server(runtimeReply("launch", "accepted", 1, "starting", 1));
        SuiteBridgeSvdrpTransport transport(configFor(server));

        SuiteBridgeHbbtvRuntimeRequest request;
        request.operation = SuiteBridgeHbbtvRuntimeOperation::Launch;
        request.sessionId = "session-a";
        request.channelId = "C-1-1051-10301";
        request.applicationId = 1;
        request.descriptorRevision = 7;

        const SuiteBridgeHbbtvCommandReply reply =
            transport.controlHbbtv(request);

        server.wait();
        assert(reply.transportSucceeded);
        assert(reply.replyCode == 250);
        assert(reply.payload.find(
            "\"capability\":\"broadcast.hbbtv.runtime\"") !=
            std::string::npos);
        assert(server.request() ==
            "PLUG suitebridge HBBRUN LAUNCH 1 session-a "
            "C-1-1051-10301 1 7\r\n");
    }

    {
        Server server(runtimeReply("input", "ok", 0, "active", 2));
        SuiteBridgeSvdrpTransport transport(configFor(server));

        SuiteBridgeHbbtvRuntimeRequest request;
        request.operation = SuiteBridgeHbbtvRuntimeOperation::Input;
        request.sessionId = "session-a";
        request.channelId = "C-1-1051-10301";
        request.applicationId = 1;
        request.descriptorRevision = 7;
        request.inputAction = SuiteBridgeHbbtvInputAction::Left;

        const SuiteBridgeHbbtvCommandReply reply =
            transport.controlHbbtv(request);

        server.wait();
        assert(reply.transportSucceeded);
        assert(server.request() ==
            "PLUG suitebridge HBBRUN INPUT 1 session-a "
            "C-1-1051-10301 1 7 LEFT\r\n");
        assert(server.request().find("VK_LEFT") == std::string::npos);
    }

    {
        SuiteBridgeSvdrpTransport transport;

        SuiteBridgeHbbtvRuntimeRequest invalid;
        invalid.operation = SuiteBridgeHbbtvRuntimeOperation::Launch;
        invalid.sessionId = "bad/session";
        invalid.channelId = "C-1-1051-10301";
        invalid.applicationId = 1;
        invalid.descriptorRevision = 7;
        assert(!transport.controlHbbtv(invalid).transportSucceeded);

        invalid.sessionId = "session-a";
        invalid.inputAction = SuiteBridgeHbbtvInputAction::Left;
        assert(!transport.controlHbbtv(invalid).transportSucceeded);

        assert(!transport.discoverHbbtv("bad channel").transportSucceeded);
        assert(!transport.discoverHbbtv("").transportSucceeded);
    }

    {
        Server server(
            "550 {\"schemaVersion\":1,\"result\":\"rejected\"}\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server));
        const SuiteBridgeHbbtvCommandReply failed =
            transport.discoverHbbtv("C-1-1051-10301");

        server.wait();
        assert(!failed.transportSucceeded);
        assert(failed.replyCode == 550);
    }

    return 0;
}

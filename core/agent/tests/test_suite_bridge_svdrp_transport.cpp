#include "SuiteBridgeSvdrpTransport.h"

#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace vdrsuite::agent;

namespace
{

using namespace std::chrono_literals;

int noSignalSendFlags()
{
#ifdef MSG_NOSIGNAL
    return MSG_NOSIGNAL;
#else
    return 0;
#endif
}

bool sendAll(const int fd, const std::string& value)
{
    std::size_t offset = 0;

    while (offset < value.size())
    {
        const ssize_t sent = send(
            fd,
            value.data() + offset,
            value.size() - offset,
            noSignalSendFlags());

        if (sent <= 0)
        {
            return false;
        }

        offset += static_cast<std::size_t>(sent);
    }

    return true;
}

void configureReceiveTimeout(const int fd)
{
    timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    assert(setsockopt(
        fd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)) == 0);
}

class ScriptedSvdrpServer
{
public:
    ScriptedSvdrpServer(
        std::string greeting,
        std::string response,
        const bool readRequest = true,
        const std::chrono::milliseconds greetingDelay = 0ms,
        const std::chrono::milliseconds responseDelay = 0ms)
        : greeting_(std::move(greeting)),
          response_(std::move(response)),
          readRequest_(readRequest),
          greetingDelay_(greetingDelay),
          responseDelay_(responseDelay)
    {
        serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
        assert(serverFd_ >= 0);

        int reuse = 1;
        assert(setsockopt(
            serverFd_,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)) == 0);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;

        assert(bind(
            serverFd_,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == 0);

        socklen_t length = sizeof(address);
        assert(getsockname(
            serverFd_,
            reinterpret_cast<sockaddr*>(&address),
            &length) == 0);

        port_ = ntohs(address.sin_port);
        assert(listen(serverFd_, 1) == 0);

        worker_ = std::thread([this]() {
            handleConnection();
        });
    }

    ~ScriptedSvdrpServer()
    {
        wait();

        if (serverFd_ >= 0)
        {
            close(serverFd_);
        }
    }

    ScriptedSvdrpServer(const ScriptedSvdrpServer&) = delete;
    ScriptedSvdrpServer& operator=(const ScriptedSvdrpServer&) = delete;

    int port() const
    {
        return port_;
    }

    void wait()
    {
        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    const std::string& request() const
    {
        return request_;
    }

    bool peerClosed() const
    {
        return peerClosed_;
    }

private:
    void handleConnection()
    {
        const int clientFd = accept(serverFd_, nullptr, nullptr);
        assert(clientFd >= 0);
        configureReceiveTimeout(clientFd);

        if (greetingDelay_.count() > 0)
        {
            std::this_thread::sleep_for(greetingDelay_);
        }

        if (!greeting_.empty())
        {
            sendAll(clientFd, greeting_);
        }

        if (readRequest_)
        {
            char buffer[256];

            while (request_.find('\n') == std::string::npos)
            {
                const ssize_t received = recv(
                    clientFd,
                    buffer,
                    sizeof(buffer),
                    0);

                if (received <= 0)
                {
                    break;
                }

                request_.append(
                    buffer,
                    static_cast<std::size_t>(received));
            }
        }

        if (responseDelay_.count() > 0)
        {
            std::this_thread::sleep_for(responseDelay_);
        }

        if (!response_.empty())
        {
            sendAll(clientFd, response_);
        }

        char finalByte = 0;
        const ssize_t finalRead = recv(clientFd, &finalByte, 1, 0);
        peerClosed_ = finalRead == 0;
        close(clientFd);
    }

    std::string greeting_;
    std::string response_;
    bool readRequest_;
    std::chrono::milliseconds greetingDelay_;
    std::chrono::milliseconds responseDelay_;
    int serverFd_ = -1;
    int port_ = 0;
    std::thread worker_;
    std::string request_;
    bool peerClosed_ = false;
};

SuiteBridgeSvdrpTransportConfig configFor(
    const int port,
    const std::chrono::milliseconds ioTimeout = 300ms)
{
    SuiteBridgeSvdrpTransportConfig config;
    config.host = "127.0.0.1";
    config.port = port;
    config.connectTimeout = 300ms;
    config.ioTimeout = ioTimeout;
    config.operationTimeout = 1000ms;
    return config;
}

int unusedLoopbackPort()
{
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    assert(bind(
        fd,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)) == 0);

    socklen_t length = sizeof(address);
    assert(getsockname(
        fd,
        reinterpret_cast<sockaddr*>(&address),
        &length) == 0);

    const int port = ntohs(address.sin_port);
    close(fd);
    return port;
}

void testDiscoverCommandAndSingleLineReply()
{
    ScriptedSvdrpServer server(
        "220 local-vdr SVDRP VideoDiskRecorder 2.7.9\r\n",
        "900 {\"discovery_schema\":1}\r\n");
    SuiteBridgeSvdrpTransport transport(configFor(server.port()));

    const SuiteBridgeCommandReply reply = transport.execute(
        SuiteBridgeLocalCommand::DiscoverSchema1);

    server.wait();
    assert(reply.transportStatus == SuiteBridgeTransportStatus::Success);
    assert(reply.replyCode == 900);
    assert(reply.payload == "{\"discovery_schema\":1}");
    assert(reply.diagnostic.empty());
    assert(server.request() == "PLUG suitebridge CAPS 1\r\n");
    assert(server.peerClosed());
}

void testSnapshotCommandAndLfNormalization()
{
    ScriptedSvdrpServer server(
        "220 local-vdr ready\n",
        "900 {\"active\":true}\n");
    SuiteBridgeSvdrpTransport transport(configFor(server.port()));

    const SuiteBridgeCommandReply reply = transport.execute(
        SuiteBridgeLocalCommand::Snapshot);

    server.wait();
    assert(reply.transportStatus == SuiteBridgeTransportStatus::Success);
    assert(reply.replyCode == 900);
    assert(reply.payload == "{\"active\":true}");
    assert(server.request() == "PLUG suitebridge SNAP\r\n");
    assert(server.peerClosed());
}

void testMultilineReply()
{
    ScriptedSvdrpServer server(
        "220 local-vdr ready\r\n",
        "900-first line\r\n900 second line\r\n");
    SuiteBridgeSvdrpTransport transport(configFor(server.port()));

    const SuiteBridgeCommandReply reply = transport.execute(
        SuiteBridgeLocalCommand::DiscoverSchema1);

    server.wait();
    assert(reply.transportStatus == SuiteBridgeTransportStatus::Success);
    assert(reply.replyCode == 900);
    assert(reply.payload == "first line\nsecond line");
}

void testRejectedReplyRemainsTransportSuccess()
{
    ScriptedSvdrpServer server(
        "220 local-vdr ready\r\n",
        "504 CAPS discovery schema unsupported\r\n");
    SuiteBridgeSvdrpTransport transport(configFor(server.port()));

    const SuiteBridgeCommandReply reply = transport.execute(
        SuiteBridgeLocalCommand::DiscoverSchema1);

    server.wait();
    assert(reply.transportStatus == SuiteBridgeTransportStatus::Success);
    assert(reply.replyCode == 504);
    assert(reply.payload == "CAPS discovery schema unsupported");
}

void testUnexpectedAndMalformedGreeting()
{
    {
        ScriptedSvdrpServer server(
            "221 closing\r\n",
            "",
            false);
        SuiteBridgeSvdrpTransport transport(configFor(server.port()));

        const SuiteBridgeCommandReply reply = transport.execute(
            SuiteBridgeLocalCommand::DiscoverSchema1);

        server.wait();
        assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
        assert(reply.diagnostic == "unexpected SVDRP greeting code");
        assert(server.request().empty());
    }

    {
        ScriptedSvdrpServer server(
            "not-an-svdrp-greeting\r\n",
            "",
            false);
        SuiteBridgeSvdrpTransport transport(configFor(server.port()));

        const SuiteBridgeCommandReply reply = transport.execute(
            SuiteBridgeLocalCommand::DiscoverSchema1);

        server.wait();
        assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
        assert(reply.diagnostic == "malformed SVDRP reply line");
        assert(server.request().empty());
    }
}

void testMalformedAndIncompleteReplies()
{
    {
        ScriptedSvdrpServer server(
            "220 local-vdr ready\r\n",
            "900?invalid\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server.port()));

        const SuiteBridgeCommandReply reply = transport.execute(
            SuiteBridgeLocalCommand::Snapshot);

        server.wait();
        assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
        assert(reply.diagnostic == "malformed SVDRP reply line");
    }

    {
        ScriptedSvdrpServer server(
            "220 local-vdr ready\r\n",
            "900-partial\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server.port()));

        const SuiteBridgeCommandReply reply = transport.execute(
            SuiteBridgeLocalCommand::Snapshot);

        server.wait();
        assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
        assert(reply.diagnostic ==
               "SVDRP connection closed before complete reply");
    }

    {
        ScriptedSvdrpServer server(
            "220 local-vdr ready\r\n",
            "900-first\r\n901 second\r\n");
        SuiteBridgeSvdrpTransport transport(configFor(server.port()));

        const SuiteBridgeCommandReply reply = transport.execute(
            SuiteBridgeLocalCommand::Snapshot);

        server.wait();
        assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
        assert(reply.diagnostic ==
               "inconsistent SVDRP multiline reply code");
    }
}

void testReplyTimeout()
{
    ScriptedSvdrpServer server(
        "220 local-vdr ready\r\n",
        "900 delayed\r\n",
        true,
        0ms,
        150ms);
    SuiteBridgeSvdrpTransport transport(configFor(server.port(), 40ms));

    const SuiteBridgeCommandReply reply = transport.execute(
        SuiteBridgeLocalCommand::Snapshot);

    server.wait();
    assert(reply.transportStatus == SuiteBridgeTransportStatus::Timeout);
    assert(reply.diagnostic == "SVDRP reply timed out");
    assert(server.peerClosed());
}

void testGreetingTimeout()
{
    ScriptedSvdrpServer server(
        "220 delayed greeting\r\n",
        "",
        false,
        150ms);
    SuiteBridgeSvdrpTransport transport(configFor(server.port(), 40ms));

    const SuiteBridgeCommandReply reply = transport.execute(
        SuiteBridgeLocalCommand::DiscoverSchema1);

    server.wait();
    assert(reply.transportStatus == SuiteBridgeTransportStatus::Timeout);
    assert(reply.diagnostic == "SVDRP greeting timed out");
    assert(server.request().empty());
    assert(server.peerClosed());
}

void testOversizedReply()
{
    const std::string oversized =
        "900 " +
        std::string(
            SuiteBridgeSvdrpTransport::MaximumReplyBytes + 32,
            'x') +
        "\r\n";
    ScriptedSvdrpServer server(
        "220 local-vdr ready\r\n",
        oversized);
    SuiteBridgeSvdrpTransport transport(configFor(server.port()));

    const SuiteBridgeCommandReply reply = transport.execute(
        SuiteBridgeLocalCommand::Snapshot);

    server.wait();
    assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
    assert(reply.diagnostic == "SVDRP reply exceeds bounded size");
}

void testUnavailableAndInvalidConfiguration()
{
    {
        SuiteBridgeSvdrpTransportConfig config;
        config.host.clear();
        SuiteBridgeSvdrpTransport transport(config);

        const SuiteBridgeCommandReply reply = transport.execute(
            SuiteBridgeLocalCommand::Snapshot);

        assert(reply.transportStatus == SuiteBridgeTransportStatus::Unavailable);
        assert(reply.diagnostic == "SVDRP transport is not configured");
    }

    {
        SuiteBridgeSvdrpTransportConfig config;
        config.port = 0;
        SuiteBridgeSvdrpTransport transport(config);

        const SuiteBridgeCommandReply reply = transport.execute(
            SuiteBridgeLocalCommand::Snapshot);

        assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
        assert(reply.diagnostic == "invalid SVDRP transport configuration");
    }

    {
        SuiteBridgeSvdrpTransportConfig config;
        config.host = "vdr.example.invalid";
        SuiteBridgeSvdrpTransport transport(config);

        const SuiteBridgeCommandReply reply = transport.execute(
            SuiteBridgeLocalCommand::Snapshot);

        assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
        assert(reply.diagnostic ==
               "SVDRP host must be localhost or a numeric address");
    }
}

void testConnectionFailure()
{
    SuiteBridgeSvdrpTransport transport(
        configFor(unusedLoopbackPort()));

    const SuiteBridgeCommandReply reply = transport.execute(
        SuiteBridgeLocalCommand::DiscoverSchema1);

    assert(reply.transportStatus == SuiteBridgeTransportStatus::Failed);
    assert(reply.diagnostic == "SVDRP endpoint connection failed");
}

}

int main()
{
    testDiscoverCommandAndSingleLineReply();
    testSnapshotCommandAndLfNormalization();
    testMultilineReply();
    testRejectedReplyRemainsTransportSuccess();
    testUnexpectedAndMalformedGreeting();
    testMalformedAndIncompleteReplies();
    testReplyTimeout();
    testGreetingTimeout();
    testOversizedReply();
    testUnavailableAndInvalidConfiguration();
    testConnectionFailure();

    return 0;
}
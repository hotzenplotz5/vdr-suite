#include "SuiteBridgeSvdrpTransport.h"

#include <arpa/inet.h>
#include <cassert>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace vdrsuite::agent;

namespace
{
class OneShotServer
{
public:
    explicit OneShotServer(std::string response)
        : response_(std::move(response))
    {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        assert(fd_ >= 0);
        int reuse = 1;
        assert(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
            &reuse, sizeof(reuse)) == 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        assert(bind(fd_, reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == 0);
        socklen_t length = sizeof(address);
        assert(getsockname(fd_, reinterpret_cast<sockaddr*>(&address),
            &length) == 0);
        port_ = ntohs(address.sin_port);
        assert(listen(fd_, 1) == 0);
        worker_ = std::thread([this] { serve(); });
    }

    ~OneShotServer()
    {
        wait();
        if (fd_ >= 0) close(fd_);
    }

    int port() const { return port_; }
    const std::string& request() const { return request_; }
    void wait()
    {
        if (worker_.joinable()) worker_.join();
    }

private:
    void serve()
    {
        const int client = accept(fd_, nullptr, nullptr);
        assert(client >= 0);
        const std::string greeting = "220 local-vdr ready\r\n";
        assert(send(client, greeting.data(), greeting.size(), 0) ==
               static_cast<ssize_t>(greeting.size()));
        char buffer[512];
        while (request_.find('\n') == std::string::npos)
        {
            const ssize_t count = recv(client, buffer, sizeof(buffer), 0);
            assert(count > 0);
            request_.append(buffer, static_cast<std::size_t>(count));
        }
        assert(send(client, response_.data(), response_.size(), 0) ==
               static_cast<ssize_t>(response_.size()));
        shutdown(client, SHUT_WR);
        close(client);
    }

    std::string response_;
    int fd_ = -1;
    int port_ = 0;
    std::thread worker_;
    std::string request_;
};

SuiteBridgeSvdrpTransportConfig config(int port)
{
    SuiteBridgeSvdrpTransportConfig value;
    value.host = "127.0.0.1";
    value.port = static_cast<std::uint16_t>(port);
    return value;
}
}

int main()
{
    {
        OneShotServer server("900 {\"nativeOperation\":\"vdr.native.probe\"}\r\n");
        SuiteBridgeSvdrpTransport transport(config(server.port()));
        const auto reply = transport.discoverNativeProbe();
        server.wait();
        assert(reply.transportSucceeded());
        assert(reply.replyCode == 900);
        assert(server.request() == "PLUG suitebridge NCAP 1\r\n");
    }

    SuiteBridgeNativeProbeRequest request;
    request.commandId = "cmd_1";
    request.requestFingerprint = "fp1_1234";
    request.operationId = "op_1";
    request.jobId = "job_1";
    request.attemptId = "att_1";
    request.claimEpoch = 1;
    request.backendId = "default";
    request.agentId = "agt_1";
    request.agentInstanceId = "agi_1";
    request.backendGeneration = 7;
    request.pluginInstanceEpoch = "pie_1";
    request.probeNonce = "pbn_1";
    {
        OneShotServer server("900 {}\r\n");
        SuiteBridgeSvdrpTransport transport(config(server.port()));
        const auto reply = transport.executeNativeProbe(request);
        server.wait();
        assert(reply.transportSucceeded());
        assert(server.request() ==
            "PLUG suitebridge NPROBE EXEC vdr-suite-native/1 "
            "vdr.native.probe 1 cmd_1 fp1_1234 op_1 job_1 att_1 1 "
            "default agt_1 agi_1 7 pie_1 1 pbn_1\r\n");
    }

    SuiteBridgeNativeProbeReadbackRequest readback;
    readback.commandId = "cmd_1";
    readback.requestFingerprint = "fp1_1234";
    readback.pluginInstanceEpoch = "pie_1";
    readback.nativeExecutionSequence = 1;
    {
        OneShotServer server("900 {}\r\n");
        SuiteBridgeSvdrpTransport transport(config(server.port()));
        const auto reply = transport.readNativeProbe(readback);
        server.wait();
        assert(reply.transportSucceeded());
        assert(server.request() ==
            "PLUG suitebridge NPROBE READ 1 cmd_1 fp1_1234 pie_1 1\r\n");
    }

    request.probeNonce = "not allowed";
    SuiteBridgeSvdrpTransport unconfigured({});
    const auto rejected = unconfigured.executeNativeProbe(request);
    assert(rejected.transportStatus == SuiteBridgeTransportStatus::Failed);
    assert(rejected.diagnostic == "invalid typed native probe request");
    return 0;
}

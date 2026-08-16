#include "SuiteBridgeNativeTimerDeleteTransport.h"

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
        char buffer[2048];
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
    value.port = port;
    return value;
}

std::string fingerprintToken(char digit = 'a')
{
    return "sha256:" + std::string(64, digit);
}

BackendAgentNativeTimerDeleteTransportRequest validRequest()
{
    BackendAgentNativeTimerDeleteTransportRequest request;
    auto& command = request.command;
    command.commandId = "cmd_1";
    command.requestFingerprint = "fp_1";
    command.operationId = "op_1";
    command.operationRevision = "opr_1";
    command.nativeTimerBindingId = "ntb_1";
    command.expectedBindingRevision = "nbr_1";
    command.expectedNativeTimerFingerprint = fingerprintToken();
    command.timerAssignmentId = "tas_1";
    command.backendNativeTimerId = "42";
    command.jobId = "job_1";
    command.attemptId = "att_1";
    command.claimEpoch = 3;
    command.backendId = "default";
    command.agentId = "agt_1";
    command.agentInstanceId = "agi_1";
    command.backendGeneration = 7;
    command.controlPlaneClaimedAt = 100;
    command.localProviderSelection.backendId = "default";
    command.localProviderSelection.authorityDomain = "vdr.timer";
    command.localProviderSelection.providerId = "suitebridge:local";
    command.localProviderSelection.providerKind = "suitebridge";
    command.localProviderSelection.ownershipGeneration = 9;
    command.localProviderSelection.providerInstanceEpoch = "pie_1";
    command.localProviderSelection.providerGeneration = 1;
    command.localProviderSelection.capabilityRevision = 1;
    command.localProviderSelection.requiredCapability = "vdr.timer.delete";
    request.localStartingPersistedAt = 101;
    return request;
}
}

int main()
{
    {
        OneShotServer server(
            "900 vdr-suite-ntdel-cap/1 vdr.timer.delete 1 timer-delete "
            "disabled suitebridge pie_1 1 1 disabled\r\n");
        SuiteBridgeNativeTimerDeleteTransport transport(config(server.port()));
        BackendAgentLocalProviderFacts facts;
        std::string reason;
        assert(transport.discoverProvider(facts, reason));
        server.wait();
        assert(reason == "native_timer_delete_suitebridge_provider_discovered_disabled");
        assert(facts.providerId == "suitebridge:local");
        assert(facts.providerKind == "suitebridge");
        assert(facts.providerInstanceEpoch == "pie_1");
        assert(facts.providerGeneration == 1);
        assert(facts.capabilityRevision == 1);
        assert(facts.available);
        assert(facts.capabilities.size() == 1);
        assert(facts.capabilities.front() == "vdr.timer.delete");
        assert(server.request() == "PLUG suitebridge NTDEL CAP 1\r\n");
    }

    {
        OneShotServer server(
            "556 vdr-suite-ntdel-result/1 cmd_1 fp_1 vdr.timer.delete 1 "
            "pie_1 1 1 rejected_without_effect disabled ntdel:disabled:cmd_1\r\n");
        SuiteBridgeNativeTimerDeleteTransport transport(config(server.port()));
        const auto request = validRequest();
        const auto reply = transport.deleteTimer(request);
        server.wait();
        assert(reply.disposition ==
            BackendAgentNativeTimerDeleteTransportDisposition::rejectedWithoutEffect);
        assert(reply.evidenceReference == "ntdel:disabled:cmd_1");
        assert(request.command.expectedNativeTimerFingerprint == fingerprintToken());
        assert(server.request() ==
            "PLUG suitebridge NTDEL EXEC vdr-suite-native/1 vdr.timer.delete 1 "
            "cmd_1 fp_1 op_1 opr_1 ntb_1 nbr_1 " +
            request.command.expectedNativeTimerFingerprint +
            " tas_1 42 job_1 att_1 3 default agt_1 agi_1 7 100 vdr.timer "
            "suitebridge:local suitebridge 9 pie_1 1 1 vdr.timer.delete 101\r\n");
    }

    {
        auto request = validRequest();
        request.command.expectedNativeTimerFingerprint.clear();
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();
        SuiteBridgeNativeTimerDeleteTransport transport(disabled);
        const auto reply = transport.deleteTimer(request);
        assert(reply.disposition ==
            BackendAgentNativeTimerDeleteTransportDisposition::rejectedWithoutEffect);
        assert(reply.evidenceReference == "suitebridge:ntdel:local-request-invalid");
    }

    {
        auto request = validRequest();
        request.command.expectedNativeTimerFingerprint.back() = 'g';
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();
        SuiteBridgeNativeTimerDeleteTransport transport(disabled);
        const auto reply = transport.deleteTimer(request);
        assert(reply.disposition ==
            BackendAgentNativeTimerDeleteTransportDisposition::rejectedWithoutEffect);
        assert(reply.evidenceReference == "suitebridge:ntdel:local-request-invalid");
    }

    {
        auto request = validRequest();
        request.command.operationRevision = "bad revision";
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();
        SuiteBridgeNativeTimerDeleteTransport transport(disabled);
        const auto reply = transport.deleteTimer(request);
        assert(reply.disposition ==
            BackendAgentNativeTimerDeleteTransportDisposition::rejectedWithoutEffect);
        assert(reply.evidenceReference == "suitebridge:ntdel:local-request-invalid");
    }

    {
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();
        SuiteBridgeNativeTimerDeleteTransport transport(disabled);
        const auto reply = transport.deleteTimer(validRequest());
        assert(reply.disposition ==
            BackendAgentNativeTimerDeleteTransportDisposition::outcomeUnknown);
        assert(reply.evidenceReference ==
            "suitebridge:ntdel:transport-outcome-unknown");
    }

    return 0;
}

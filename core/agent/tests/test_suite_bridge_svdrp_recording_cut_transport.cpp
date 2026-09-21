#include "SuiteBridgeRecordingCutTransport.h"

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
        char buffer[4096];
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

BackendAgentRecordingCutTransportRequest validRequest()
{
    BackendAgentRecordingCutTransportRequest request;
    auto& command = request.command;
    command.commandId = "cmd_cut_1";
    command.requestFingerprint = "fp1_aaaaaaaaaaaaaaaa";
    command.operationId = "op_cut_1";
    command.operationRevision = "opr_1";
    command.recordingKey = "0123456789abcdef0123456789abcdef";
    command.expectedMarksRevision = "fedcba9876543210fedcba9876543210";
    command.jobId = "job_cut_1";
    command.attemptId = "att_cut_1";
    command.claimEpoch = 3;
    command.backendId = "default";
    command.agentId = "agt_1";
    command.agentInstanceId = "agi_1";
    command.backendGeneration = 7;
    command.controlPlaneClaimedAt = 100;
    command.localProviderSelection.backendId = "default";
    command.localProviderSelection.authorityDomain =
        kBackendAgentRecordingCutAuthorityDomain;
    command.localProviderSelection.providerId = kBackendAgentRecordingCutProviderId;
    command.localProviderSelection.providerKind = kBackendAgentRecordingCutProviderKind;
    command.localProviderSelection.ownershipGeneration = 9;
    command.localProviderSelection.providerInstanceEpoch = "pie_1";
    command.localProviderSelection.providerGeneration = 1;
    command.localProviderSelection.capabilityRevision = 1;
    command.localProviderSelection.requiredCapability = kBackendAgentRecordingCutCapability;
    request.localStartingPersistedAt = 101;
    return request;
}
}

int main()
{
    {
        OneShotServer server(
            "900 vdr-suite-ncut-cap/1 vdr.recording.cut 1 "
            "recording-cut disabled suitebridge pie_1 1 1 disabled\r\n");
        SuiteBridgeRecordingCutTransport transport(config(server.port()));
        BackendAgentLocalProviderFacts facts;
        std::string reason;
        assert(transport.discoverProvider(facts, reason));
        server.wait();
        assert(!facts.available);
        assert(facts.providerId == kBackendAgentRecordingCutProviderId);
        assert(facts.capabilities == std::vector<std::string>{
            kBackendAgentRecordingCutCapability});
        assert(server.request() == "PLUG suitebridge NCUT CAP 1 start\r\n");
    }

    {
        OneShotServer server(
            "900 vdr-suite-ncut-cap/1 vdr.recording.cut 1 "
            "recording-cut enabled suitebridge pie_1 1 1 enabled\r\n");
        SuiteBridgeRecordingCutTransport transport(config(server.port()));
        BackendAgentLocalProviderFacts facts;
        std::string reason;
        assert(transport.discoverProvider(facts, reason));
        server.wait();
        assert(facts.available);
        assert(facts.providerInstanceEpoch == "pie_1");
    }

    {
        OneShotServer server(
            "557 vdr-suite-ncut-result/1 cmd_cut_1 fp1_aaaaaaaaaaaaaaaa "
            "vdr.recording.cut 1 pie_1 1 1 accepted_unverified "
            "callback_applied ncut:vdr:queued:cmd_cut_1\r\n");
        SuiteBridgeRecordingCutTransport transport(config(server.port()));
        const auto reply = transport.startCut(validRequest());
        server.wait();
        assert(reply.disposition ==
            BackendAgentRecordingCutTransportDisposition::acceptedUnverified);
        assert(reply.evidenceReference == "ncut:vdr:queued:cmd_cut_1");
        assert(server.request() ==
            "PLUG suitebridge NCUT EXEC vdr-suite-native/1 vdr.recording.cut 1 "
            "cmd_cut_1 fp1_aaaaaaaaaaaaaaaa op_cut_1 opr_1 "
            "0123456789abcdef0123456789abcdef "
            "fedcba9876543210fedcba9876543210 job_cut_1 att_cut_1 3 "
            "default agt_1 agi_1 7 100 vdr.recording.cut "
            "suitebridge:recording-cut suitebridge 9 pie_1 1 1 "
            "vdr.recording.cut 101\r\n");
    }

    {
        OneShotServer server(
            "558 vdr-suite-ncut-result/1 cmd_cut_1 fp1_aaaaaaaaaaaaaaaa "
            "vdr.recording.cut 1 pie_1 1 1 outcome_unknown callback_unknown "
            "ncut:vdr:exception:cmd_cut_1\r\n");
        SuiteBridgeRecordingCutTransport transport(config(server.port()));
        const auto reply = transport.startCut(validRequest());
        server.wait();
        assert(reply.disposition ==
            BackendAgentRecordingCutTransportDisposition::outcomeUnknown);
    }

    {
        OneShotServer server(
            "557 vdr-suite-ncut-result/1 cmd_cut_1 fp1_aaaaaaaaaaaaaaaa "
            "vdr.recording.cut 1 pie_other 1 1 accepted_unverified "
            "callback_applied ncut:vdr:queued:cmd_cut_1\r\n");
        SuiteBridgeRecordingCutTransport transport(config(server.port()));
        const auto reply = transport.startCut(validRequest());
        server.wait();
        assert(reply.disposition ==
            BackendAgentRecordingCutTransportDisposition::outcomeUnknown);
        assert(reply.evidenceReference == "suitebridge:ncut:reply-fence-mismatch");
    }

    {
        OneShotServer server(
            "555 vdr-suite-ncut-result/1 cmd_cut_1 fp1_aaaaaaaaaaaaaaaa "
            "vdr.recording.cut 1 pie_old 1 1 rejected_without_effect stale "
            "ncut:stale:cmd_cut_1\r\n");
        SuiteBridgeRecordingCutTransport transport(config(server.port()));
        const auto reply = transport.startCut(validRequest());
        server.wait();
        assert(reply.disposition ==
            BackendAgentRecordingCutTransportDisposition::rejectedWithoutEffect);
    }

    {
        auto request = validRequest();
        request.command.expectedMarksRevision = "bad";
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();
        SuiteBridgeRecordingCutTransport transport(disabled);
        const auto reply = transport.startCut(request);
        assert(reply.disposition ==
            BackendAgentRecordingCutTransportDisposition::rejectedWithoutEffect);
        assert(reply.evidenceReference == "suitebridge:ncut:local-request-invalid");
    }

    return 0;
}

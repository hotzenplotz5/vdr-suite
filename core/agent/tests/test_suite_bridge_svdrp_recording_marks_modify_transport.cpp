#include "SuiteBridgeRecordingMarksModifyTransport.h"

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

std::string fingerprintToken(char digit = 'a')
{
    return "sha256:" + std::string(64, digit);
}

BackendAgentRecordingMarksModifyTransportRequest validRequest(
    BackendAgentRecordingMarksModifyKind kind =
        BackendAgentRecordingMarksModifyKind::replace)
{
    BackendAgentRecordingMarksModifyTransportRequest request;
    auto& command = request.command;
    command.kind = kind;
    command.commandId = "cmd_1";
    command.requestFingerprint = fingerprintToken();
    command.operationId = "op_1";
    command.operationRevision = "opr_1";
    command.recordingKey = "0123456789abcdef0123456789abcdef";
    command.expectedMarksRevision = "fedcba9876543210fedcba9876543210";
    if (kind == BackendAgentRecordingMarksModifyKind::add)
        command.targetFrame = 125;
    else if (kind == BackendAgentRecordingMarksModifyKind::deleteMark)
        command.sourceFrame = 125;
    else if (kind == BackendAgentRecordingMarksModifyKind::move)
    {
        command.sourceFrame = 125;
        command.targetFrame = 250;
    }
    else if (kind == BackendAgentRecordingMarksModifyKind::replace)
        command.replacementFrames = {125, 250, 375};
    command.jobId = "job_1";
    command.attemptId = "att_1";
    command.claimEpoch = 3;
    command.backendId = "default";
    command.agentId = "agt_1";
    command.agentInstanceId = "agi_1";
    command.backendGeneration = 7;
    command.controlPlaneClaimedAt = 100;
    command.localProviderSelection.backendId = "default";
    command.localProviderSelection.authorityDomain =
        kBackendAgentRecordingMarksModifyAuthorityDomain;
    command.localProviderSelection.providerId =
        kBackendAgentRecordingMarksModifyProviderId;
    command.localProviderSelection.providerKind =
        kBackendAgentRecordingMarksModifyProviderKind;
    command.localProviderSelection.ownershipGeneration = 9;
    command.localProviderSelection.providerInstanceEpoch = "pie_1";
    command.localProviderSelection.providerGeneration = 1;
    command.localProviderSelection.capabilityRevision = 2;
    command.localProviderSelection.requiredCapability =
        kBackendAgentRecordingMarksModifyCapability;
    request.localStartingPersistedAt = 101;
    return request;
}
}

int main()
{
    {
        OneShotServer server(
            "900 vdr-suite-nmarks-cap/2 vdr.recording.marks.modify 2 "
            "recording-marks-modify disabled suitebridge pie_1 1 2 disabled\r\n");
        SuiteBridgeRecordingMarksModifyTransport transport(config(server.port()));
        BackendAgentLocalProviderFacts facts;
        std::string reason;
        assert(transport.discoverProvider(facts, reason));
        server.wait();
        assert(reason ==
            "recording_marks_modify_suitebridge_provider_discovered_disabled");
        assert(facts.providerId == "suitebridge:local");
        assert(facts.providerKind == "suitebridge");
        assert(facts.providerInstanceEpoch == "pie_1");
        assert(facts.providerGeneration == 1);
        assert(facts.capabilityRevision == 2);
        assert(!facts.available);
        assert(facts.capabilities.size() == 1);
        assert(facts.capabilities.front() == "vdr.recording.marks.modify");
        assert(server.request() == "PLUG suitebridge NMARKS CAP 2 modify\r\n");
    }

    {
        OneShotServer server(
            "900 vdr-suite-nmarks-cap/2 vdr.recording.marks.modify 2 "
            "recording-marks-modify enabled suitebridge pie_1 1 2 enabled\r\n");
        SuiteBridgeRecordingMarksModifyTransport transport(config(server.port()));
        BackendAgentLocalProviderFacts facts;
        std::string reason;
        assert(transport.discoverProvider(facts, reason));
        server.wait();
        assert(reason ==
            "recording_marks_modify_suitebridge_provider_discovered_enabled");
        assert(facts.available);
    }

    {
        OneShotServer server(
            "557 vdr-suite-nmarks-result/2 cmd_1 " + fingerprintToken() +
            " vdr.recording.marks.modify 2 pie_1 1 2 accepted_unverified "
            "callback_applied nmarks:vdr:modified-unverified:cmd_1\r\n");
        SuiteBridgeRecordingMarksModifyTransport transport(config(server.port()));
        const auto request = validRequest();
        const auto reply = transport.modifyMarks(request);
        server.wait();
        assert(reply.disposition ==
            BackendAgentRecordingMarksModifyTransportDisposition::acceptedUnverified);
        assert(reply.evidenceReference ==
            "nmarks:vdr:modified-unverified:cmd_1");
        assert(server.request() ==
            "PLUG suitebridge NMARKS EXEC vdr-suite-native/1 "
            "vdr.recording.marks.modify 2 cmd_1 " + fingerprintToken() +
            " op_1 opr_1 0123456789abcdef0123456789abcdef "
            "fedcba9876543210fedcba9876543210 replace - - 125,250,375 "
            "job_1 att_1 3 default agt_1 agi_1 7 100 vdr.recording.marks "
            "suitebridge:local suitebridge 9 pie_1 1 2 "
            "vdr.recording.marks.modify 101\r\n");
    }

    {
        OneShotServer server(
            "558 vdr-suite-nmarks-result/2 cmd_1 " + fingerprintToken() +
            " vdr.recording.marks.modify 2 pie_1 1 2 outcome_unknown "
            "callback_unknown nmarks:vdr:exception:cmd_1\r\n");
        SuiteBridgeRecordingMarksModifyTransport transport(config(server.port()));
        const auto reply = transport.modifyMarks(validRequest());
        server.wait();
        assert(reply.disposition ==
            BackendAgentRecordingMarksModifyTransportDisposition::outcomeUnknown);
        assert(reply.evidenceReference == "nmarks:vdr:exception:cmd_1");
    }

    {
        OneShotServer server(
            "557 vdr-suite-nmarks-result/2 cmd_1 " + fingerprintToken() +
            " vdr.recording.marks.modify 2 pie_other 1 2 accepted_unverified "
            "callback_applied nmarks:vdr:modified-unverified:cmd_1\r\n");
        SuiteBridgeRecordingMarksModifyTransport transport(config(server.port()));
        const auto reply = transport.modifyMarks(validRequest());
        server.wait();
        assert(reply.disposition ==
            BackendAgentRecordingMarksModifyTransportDisposition::outcomeUnknown);
        assert(reply.evidenceReference == "suitebridge:nmarks:reply-fence-mismatch");
    }

    {
        OneShotServer server(
            "555 vdr-suite-nmarks-result/2 cmd_1 " + fingerprintToken() +
            " vdr.recording.marks.modify 2 pie_old 1 2 rejected_without_effect "
            "stale nmarks:stale:cmd_1\r\n");
        SuiteBridgeRecordingMarksModifyTransport transport(config(server.port()));
        const auto reply = transport.modifyMarks(validRequest());
        server.wait();
        assert(reply.disposition ==
            BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect);
        assert(reply.evidenceReference == "nmarks:stale:cmd_1");
    }

    {
        auto request = validRequest();
        request.command.expectedMarksRevision = "not-a-revision";
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();
        SuiteBridgeRecordingMarksModifyTransport transport(disabled);
        const auto reply = transport.modifyMarks(request);
        assert(reply.disposition ==
            BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect);
        assert(reply.evidenceReference == "suitebridge:nmarks:local-request-invalid");
    }

    {
        auto request = validRequest();
        request.command.requestFingerprint.back() = 'g';
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();
        SuiteBridgeRecordingMarksModifyTransport transport(disabled);
        const auto reply = transport.modifyMarks(request);
        assert(reply.disposition ==
            BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect);
        assert(reply.evidenceReference == "suitebridge:nmarks:local-request-invalid");
    }

    {
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();
        SuiteBridgeRecordingMarksModifyTransport transport(disabled);
        const auto reply = transport.modifyMarks(validRequest());
        assert(reply.disposition ==
            BackendAgentRecordingMarksModifyTransportDisposition::outcomeUnknown);
        assert(reply.evidenceReference ==
            "suitebridge:nmarks:transport-outcome-unknown");
    }

    return 0;
}

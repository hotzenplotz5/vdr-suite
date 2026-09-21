#include "SuiteBridgeNativeTimerCreateTransport.h"

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
        assert(setsockopt(
            fd_,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)) == 0);

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

        worker_ = std::thread([this] {
            serve();
        });
    }

    ~OneShotServer()
    {
        wait();

        if (fd_ >= 0)
            close(fd_);
    }

    int port() const
    {
        return port_;
    }

    const std::string& request() const
    {
        return request_;
    }

    void wait()
    {
        if (worker_.joinable())
            worker_.join();
    }

private:
    void serve()
    {
        const int client =
            accept(fd_, nullptr, nullptr);

        assert(client >= 0);

        const std::string greeting =
            "220 local-vdr ready\r\n";

        assert(send(
            client,
            greeting.data(),
            greeting.size(),
            0) ==
            static_cast<ssize_t>(greeting.size()));

        char buffer[4096];

        while (request_.find('\n') == std::string::npos)
        {
            const ssize_t count =
                recv(client, buffer, sizeof(buffer), 0);

            assert(count > 0);

            request_.append(
                buffer,
                static_cast<std::size_t>(count));
        }

        assert(send(
            client,
            response_.data(),
            response_.size(),
            0) ==
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

std::string hexToken(const std::string& input)
{
    if (input.empty())
        return "-";

    static constexpr char Digits[] =
        "0123456789abcdef";

    std::string output;

    for (unsigned char character : input)
    {
        output.push_back(
            Digits[(character >> 4U) & 0x0fU]);
        output.push_back(
            Digits[character & 0x0fU]);
    }

    return output;
}

BackendAgentNativeTimerCreateTransportRequest validRequest()
{
    BackendAgentNativeTimerCreateTransportRequest request;
    auto& command = request.command;

    command.commandId = "cmd_1";
    command.requestFingerprint = "fp_1";
    command.operationId = "op_1";
    command.operationRevision = "opr_1";
    command.timerAssignmentId = "tas_1";
    command.expectedAssignmentRevision = "tar_1";
    command.expectedIntentRevision = "tir_1";
    command.assignmentEpoch = 4;
    command.nativeTimerBindingId = "ntb_1";
    command.jobId = "job_1";
    command.attemptId = "att_1";
    command.claimEpoch = 3;
    command.backendId = "default";
    command.agentId = "agt_1";
    command.agentInstanceId = "agi_1";
    command.backendGeneration = 7;
    command.controlPlaneClaimedAt = 100;

    command.specification.channelId =
        "S19.2E-1-1011-11100";
    command.specification.title =
        "Tagesschau 20 Uhr";
    command.specification.directory =
        "TV News";
    command.specification.day =
        "2026-08-16";
    command.specification.weekdays =
        "-------";
    command.specification.startTime =
        "2000";
    command.specification.endTime =
        "2020";
    command.specification.priority = 50;
    command.specification.lifetime = 99;
    command.specification.enabled = true;
    command.specification.vps = false;

    command.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(
            command.specification);

    command.localProviderSelection.backendId =
        "default";
    command.localProviderSelection.authorityDomain =
        "vdr.timer";
    command.localProviderSelection.providerId =
        "suitebridge:local";
    command.localProviderSelection.providerKind =
        "suitebridge";
    command.localProviderSelection.ownershipGeneration =
        9;
    command.localProviderSelection.providerInstanceEpoch =
        "pie_1";
    command.localProviderSelection.providerGeneration =
        1;
    command.localProviderSelection.capabilityRevision =
        1;
    command.localProviderSelection.requiredCapability =
        "vdr.timer.create";

    request.localStartingPersistedAt = 101;

    return request;
}

} // namespace

int main()
{
    {
        OneShotServer server(
            "900 vdr-suite-ntcreate-cap/1 "
            "vdr.timer.create 1 timer-create "
            "disabled suitebridge pie_1 1 1 disabled\r\n");

        SuiteBridgeNativeTimerCreateTransport transport(
            config(server.port()));

        BackendAgentLocalProviderFacts facts;
        std::string reason;

        assert(transport.discoverProvider(
            facts,
            reason));

        server.wait();

        assert(reason ==
            "native_timer_create_suitebridge_provider_discovered_disabled");

        assert(facts.providerId ==
            "suitebridge:local");
        assert(facts.providerKind ==
            "suitebridge");
        assert(facts.providerInstanceEpoch ==
            "pie_1");
        assert(facts.providerGeneration == 1);
        assert(facts.capabilityRevision == 1);
        assert(!facts.available);
        assert(facts.capabilities.size() == 1);
        assert(facts.capabilities.front() ==
            "vdr.timer.create");

        assert(server.request() ==
            "PLUG suitebridge NTCREATE CAP 1\r\n");
    }

    {
        OneShotServer server(
            "900 vdr-suite-ntcreate-cap/1 "
            "vdr.timer.create 1 timer-create "
            "enabled suitebridge pie_1 1 1 enabled\r\n");

        SuiteBridgeNativeTimerCreateTransport transport(
            config(server.port()));

        BackendAgentLocalProviderFacts facts;
        std::string reason;

        assert(transport.discoverProvider(
            facts,
            reason));

        server.wait();

        assert(reason ==
            "native_timer_create_suitebridge_provider_discovered_enabled");
    }

    {
        OneShotServer server(
            "556 vdr-suite-ntcreate-result/1 "
            "cmd_1 fp_1 vdr.timer.create 1 "
            "pie_1 1 1 rejected_without_effect "
            "disabled ntcreate:disabled:cmd_1\r\n");

        SuiteBridgeNativeTimerCreateTransport transport(
            config(server.port()));

        const auto request = validRequest();
        const auto reply =
            transport.createTimer(request);

        server.wait();

        assert(reply.disposition ==
            BackendAgentNativeTimerCreateTransportDisposition::
                rejectedWithoutEffect);

        assert(reply.evidenceReference ==
            "ntcreate:disabled:cmd_1");

        const auto& command = request.command;
        const auto& spec = command.specification;

        const std::string expected =
            std::string(
                "PLUG suitebridge NTCREATE EXEC "
                "vdr-suite-native/1 vdr.timer.create 1 ") +
            command.commandId + " " +
            command.requestFingerprint + " " +
            command.operationId + " " +
            command.operationRevision + " " +
            command.timerAssignmentId + " " +
            command.expectedAssignmentRevision + " " +
            command.expectedIntentRevision + " " +
            std::to_string(command.assignmentEpoch) + " " +
            command.nativeTimerBindingId + " " +
            hexToken(
                command.expectedSpecificationFingerprint) + " " +
            command.jobId + " " +
            command.attemptId + " " +
            std::to_string(command.claimEpoch) + " " +
            command.backendId + " " +
            command.agentId + " " +
            command.agentInstanceId + " " +
            std::to_string(command.backendGeneration) + " " +
            std::to_string(command.controlPlaneClaimedAt) + " " +
            command.localProviderSelection.authorityDomain + " " +
            command.localProviderSelection.providerId + " " +
            command.localProviderSelection.providerKind + " " +
            std::to_string(
                command.localProviderSelection.ownershipGeneration) + " " +
            command.localProviderSelection.providerInstanceEpoch + " " +
            std::to_string(
                command.localProviderSelection.providerGeneration) + " " +
            std::to_string(
                command.localProviderSelection.capabilityRevision) + " " +
            command.localProviderSelection.requiredCapability + " " +
            std::to_string(request.localStartingPersistedAt) + " " +
            hexToken(spec.channelId) + " " +
            hexToken(spec.title) + " " +
            hexToken(spec.directory) + " " +
            hexToken(spec.day) + " " +
            hexToken(spec.weekdays) + " " +
            hexToken(spec.startTime) + " " +
            hexToken(spec.endTime) + " " +
            std::to_string(spec.priority) + " " +
            std::to_string(spec.lifetime) + " 1 0\r\n";

        assert(server.request() == expected);

        assert(server.request().find(
            "Tagesschau 20 Uhr") ==
            std::string::npos);

        assert(server.request().find(
            "TV News") ==
            std::string::npos);
    }

    {
        OneShotServer server(
            "557 vdr-suite-ntcreate-result/1 "
            "cmd_1 fp_1 vdr.timer.create 1 "
            "pie_1 1 1 accepted_unverified "
            "callback_applied "
            "ntcreate:vdr:created-unverified:cmd_1\r\n");

        SuiteBridgeNativeTimerCreateTransport transport(
            config(server.port()));

        const auto reply =
            transport.createTimer(validRequest());

        server.wait();

        assert(reply.disposition ==
            BackendAgentNativeTimerCreateTransportDisposition::
                acceptedUnverified);

        assert(reply.evidenceReference ==
            "ntcreate:vdr:created-unverified:cmd_1");
    }

    {
        OneShotServer server(
            "558 vdr-suite-ntcreate-result/1 "
            "cmd_1 fp_1 vdr.timer.create 1 "
            "pie_1 1 1 outcome_unknown "
            "callback_unknown "
            "ntcreate:vdr:exception:cmd_1\r\n");

        SuiteBridgeNativeTimerCreateTransport transport(
            config(server.port()));

        const auto reply =
            transport.createTimer(validRequest());

        server.wait();

        assert(reply.disposition ==
            BackendAgentNativeTimerCreateTransportDisposition::
                outcomeUnknown);

        assert(reply.evidenceReference ==
            "ntcreate:vdr:exception:cmd_1");
    }

    {
        OneShotServer server(
            "557 vdr-suite-ntcreate-result/1 "
            "cmd_1 fp_1 vdr.timer.create 1 "
            "pie_other 1 1 accepted_unverified "
            "callback_applied "
            "ntcreate:vdr:created-unverified:cmd_1\r\n");

        SuiteBridgeNativeTimerCreateTransport transport(
            config(server.port()));

        const auto reply =
            transport.createTimer(validRequest());

        server.wait();

        assert(reply.disposition ==
            BackendAgentNativeTimerCreateTransportDisposition::
                outcomeUnknown);

        assert(reply.evidenceReference ==
            "suitebridge:ntcreate:reply-fence-mismatch");
    }

    {
        OneShotServer server(
            "555 vdr-suite-ntcreate-result/1 "
            "cmd_1 fp_1 vdr.timer.create 1 "
            "pie_other 1 1 rejected_without_effect "
            "stale ntcreate:stale:cmd_1\r\n");

        SuiteBridgeNativeTimerCreateTransport transport(
            config(server.port()));

        const auto reply =
            transport.createTimer(validRequest());

        server.wait();

        assert(reply.disposition ==
            BackendAgentNativeTimerCreateTransportDisposition::
                rejectedWithoutEffect);

        assert(reply.evidenceReference ==
            "ntcreate:stale:cmd_1");
    }

    {
        auto request = validRequest();
        request.command.specification.title =
            "changed after fingerprint";

        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();

        SuiteBridgeNativeTimerCreateTransport transport(
            disabled);

        const auto reply =
            transport.createTimer(request);

        assert(reply.disposition ==
            BackendAgentNativeTimerCreateTransportDisposition::
                rejectedWithoutEffect);

        assert(reply.evidenceReference ==
            "suitebridge:ntcreate:local-request-invalid");
    }

    {
        auto request = validRequest();
        request.command.operationRevision =
            "bad revision";

        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();

        SuiteBridgeNativeTimerCreateTransport transport(
            disabled);

        const auto reply =
            transport.createTimer(request);

        assert(reply.disposition ==
            BackendAgentNativeTimerCreateTransportDisposition::
                rejectedWithoutEffect);
    }

    {
        SuiteBridgeSvdrpTransportConfig disabled;
        disabled.host.clear();

        SuiteBridgeNativeTimerCreateTransport transport(
            disabled);

        const auto reply =
            transport.createTimer(validRequest());

        assert(reply.disposition ==
            BackendAgentNativeTimerCreateTransportDisposition::
                outcomeUnknown);

        assert(reply.evidenceReference ==
            "suitebridge:ntcreate:transport-outcome-unknown");
    }

    return 0;
}

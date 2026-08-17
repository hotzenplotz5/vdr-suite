#include "BackendAgentClient.h"
#include "BackendAgentCommand.h"
#include "BackendAgentCommandClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentNativeTimerCreateExecutor.h"
#include "BackendAgentNativeTimerDeleteExecutor.h"
#include "BackendAgentNativeTimerModifyExecutor.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
class NoTransport final : public IBackendAgentControlPlaneTransport
{
public:
    int calls = 0;
    BackendAgentTransportResponse postEnrollment(const std::string&,const std::string&,const std::string&,const std::string&) override
    {
        ++calls; return {};
    }
    BackendAgentTransportResponse postAuthenticated(const std::string&,const std::string&,const std::string&,const std::string&) override
    {
        ++calls; return {};
    }
};

BackendAgentCommandAssignment assignment()
{
    BackendAgentCommandAssignment value;
    value.present=true;value.requestId="req_restart";value.correlationId="corr_restart";
    value.operationId="op_restart";value.jobId="job_restart";value.attemptId="att_restart";
    value.claimEpoch=1;value.commandId="cmd_restart";value.backendId="default";
    value.agentId="agt_client";value.agentInstanceId="agi_old";value.backendGeneration=7;
    value.commandType="probe.noop";value.payloadVersion=1;value.payload="{}";
    value.verificationPolicy="none";value.assignedAt=100;value.deadline=500;
    value.requestFingerprint=backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

void writeState(const std::string& path,bool resultAcknowledged)
{
    const auto value=assignment();
    std::ofstream out(path);
    out<<"version=1\nprotocol_version="<<value.protocolVersion
       <<"\nrequest_id="<<value.requestId<<"\ncorrelation_id="<<value.correlationId
       <<"\noperation_id="<<value.operationId<<"\njob_id="<<value.jobId
       <<"\nattempt_id="<<value.attemptId<<"\nclaim_epoch="<<value.claimEpoch
       <<"\ncommand_id="<<value.commandId<<"\nbackend_id="<<value.backendId
       <<"\nagent_id="<<value.agentId<<"\nagent_instance_id="<<value.agentInstanceId
       <<"\nbackend_generation="<<value.backendGeneration<<"\ncommand_type="<<value.commandType
       <<"\npayload_version="<<value.payloadVersion<<"\npayload="<<value.payload
       <<"\nrequest_fingerprint="<<value.requestFingerprint
       <<"\nverification_policy="<<value.verificationPolicy
       <<"\nassigned_at="<<value.assignedAt<<"\ndeadline="<<value.deadline
       <<"\nreceipt_category=accepted\nreceived_at=110\nreceipt_reason=durably_recorded"
       <<"\nreceipt_acknowledged=1\ndispatch_state=effect_reported\nresult_present=1"
       <<"\nresult_acknowledged="<<(resultAcknowledged?1:0)
       <<"\nverification_state=not_required\nresult_category=succeeded"
       <<"\nerror_category=none\nretry_classification=none"
       <<"\nbounded_diagnostics=probe.noop completed without native side effect"
       <<"\ncompleted_at=120\n";
    out.close();
    assert(chmod(path.c_str(),0600)==0);
}

using vdrsuite::agent::BackendAgentLocalProviderFacts;

BackendAgentLocalProviderFacts timerFacts(
    std::vector<std::string> capabilities,
    const std::string& epoch = "pie_timer_activation",
    bool available = true)
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = "suitebridge:local";
    facts.providerKind = "suitebridge";
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = 1;
    facts.capabilityRevision = 1;
    facts.available = available;
    facts.capabilities = std::move(capabilities);
    return facts;
}

class PollTransport final : public IBackendAgentControlPlaneTransport
{
public:
    std::string requestBody;
    BackendAgentTransportResponse postEnrollment(
        const std::string&, const std::string&, const std::string&,
        const std::string&) override
    {
        return {};
    }
    BackendAgentTransportResponse postAuthenticated(
        const std::string&, const std::string&, const std::string& path,
        const std::string& body) override
    {
        assert(path == "/api/agent/v1/commands/poll");
        requestBody = body;
        return {
            true, 200,
            "{\"hasAssignment\":false,\"reasonCode\":\"no_command_available\"}",
            ""};
    }
};

class CreateTransport final
    : public vdrsuite::agent::IBackendAgentNativeTimerCreateTransport
{
public:
    BackendAgentLocalProviderFacts facts = timerFacts({"vdr.timer.create"});
    bool discoverProvider(
        BackendAgentLocalProviderFacts& value, std::string& reason) override
    {
        value = facts; reason = facts.available ? "enabled" : "disabled";
        return true;
    }
    vdrsuite::agent::BackendAgentNativeTimerCreateTransportReply createTimer(
        const vdrsuite::agent::BackendAgentNativeTimerCreateTransportRequest&)
        override
    {
        return {};
    }
};

class DeleteTransport final
    : public vdrsuite::agent::IBackendAgentNativeTimerDeleteTransport
{
public:
    BackendAgentLocalProviderFacts facts = timerFacts({"vdr.timer.delete"});
    bool discoverProvider(
        BackendAgentLocalProviderFacts& value, std::string& reason) override
    {
        value = facts; reason = facts.available ? "enabled" : "disabled";
        return true;
    }
    vdrsuite::agent::BackendAgentNativeTimerDeleteTransportReply deleteTimer(
        const vdrsuite::agent::BackendAgentNativeTimerDeleteTransportRequest&)
        override
    {
        return {};
    }
};

class ModifyTransport final
    : public vdrsuite::agent::IBackendAgentNativeTimerModifyTransport
{
public:
    BackendAgentLocalProviderFacts facts =
        timerFacts({"vdr.timer.update", "vdr.timer.toggle"});
    bool discoverProvider(
        BackendAgentLocalProviderFacts& value, std::string& reason) override
    {
        value = facts; reason = facts.available ? "enabled" : "disabled";
        return true;
    }
    vdrsuite::agent::BackendAgentNativeTimerModifyTransportReply modifyTimer(
        const vdrsuite::agent::BackendAgentNativeTimerModifyTransportRequest&)
        override
    {
        return {};
    }
};

BackendAgentCommandPollRequest capturePoll(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    PollTransport& transport,
    std::string& reason)
{
    assert(pollBackendAgentCommand(config, context, transport, reason));
    BackendAgentCommandPollRequest request;
    assert(parseBackendAgentCommandPollRequestJson(
        transport.requestBody, request, reason));
    return request;
}

void testTimerAdvertisementActivation()
{
    const std::string path = "/tmp/vdr-suite-command-activation-state";
    std::remove(path.c_str());

    CreateTransport create;
    DeleteTransport remove;
    ModifyTransport modify;
    BackendAgentCommandClientConfig config;
    config.statePath = path;
    config.commandTypes = {
        "vdr.timer.create", "vdr.timer.update",
        "vdr.timer.toggle", "vdr.timer.delete"};
    config.nativeTimerCreateTransport = &create;
    config.nativeTimerDeleteTransport = &remove;
    config.nativeTimerModifyTransport = &modify;
    const BackendAgentCommandClientContext context{
        "agt_client", "secret-material-at-least-thirty-two-bytes",
        "default", "agi_current", 8};
    std::string reason;

    PollTransport enabledTransport;
    const auto enabled = capturePoll(config, context, enabledTransport, reason);
    assert(enabled.supportedCommandTypes == config.commandTypes);
    assert(enabled.localProviders.size() == 1);
    const auto& provider = enabled.localProviders.front();
    for (const std::string& capability : config.commandTypes)
        assert(std::find(
            provider.capabilities.begin(), provider.capabilities.end(),
            capability) != provider.capabilities.end());

    create.facts.available = false;
    PollTransport partialTransport;
    const auto partial = capturePoll(config, context, partialTransport, reason);
    assert(partial.supportedCommandTypes ==
        std::vector<std::string>({
            "vdr.timer.update", "vdr.timer.toggle", "vdr.timer.delete"}));
    assert(partial.localProviders.size() == 1);
    assert(std::find(
        partial.localProviders.front().capabilities.begin(),
        partial.localProviders.front().capabilities.end(),
        "vdr.timer.create") ==
        partial.localProviders.front().capabilities.end());

    create.facts.available = true;
    modify.facts.providerInstanceEpoch = "pie_timer_activation_replaced";
    PollTransport incoherentTransport;
    const auto incoherent =
        capturePoll(config, context, incoherentTransport, reason);
    assert(incoherent.supportedCommandTypes.empty());
    assert(incoherent.localProviders.empty());

    std::remove(path.c_str());
}
}

int main()
{
    testTimerAdvertisementActivation();
    const std::string path="/tmp/vdr-suite-command-restart-state";
    std::remove(path.c_str());
    BackendAgentCommandClientConfig config{path,{"probe.noop"}};
    BackendAgentCommandClientContext current{"agt_client","secret-material-at-least-thirty-two-bytes","default","agi_new",8};
    NoTransport transport;
    std::string reason;

    writeState(path,true);
    assert(reconcileBackendAgentCommandState(config,current,transport,reason));
    assert(reason=="completed_command_state_retired");
    assert(access(path.c_str(),F_OK)!=0);
    assert(transport.calls==0);

    writeState(path,false);
    assert(!reconcileBackendAgentCommandState(config,current,transport,reason));
    assert(reason=="local_command_generation_fenced");
    assert(access(path.c_str(),F_OK)==0);
    assert(transport.calls==0);
    std::remove(path.c_str());
    return 0;
}

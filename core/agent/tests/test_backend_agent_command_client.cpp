#include "BackendAgentClient.h"
#include "BackendAgentCommand.h"
#include "BackendAgentCommandClient.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

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
}

int main()
{
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

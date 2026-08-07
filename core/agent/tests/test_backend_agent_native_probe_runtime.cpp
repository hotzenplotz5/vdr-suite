#include "BackendAgentClient.h"
#include "BackendAgentCommand.h"
#include "BackendAgentCommandClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentNativeProbe.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace vdrsuite::agent;

namespace {
BackendAgentCommandAssignment makeAssignment()
{
    BackendAgentCommandAssignment a;
    a.present=true;a.requestId="req_1";a.correlationId="corr_1";a.operationId="op_1";
    a.jobId="job_1";a.attemptId="att_1";a.claimEpoch=1;a.commandId="cmd_1";
    a.backendId="default";a.agentId="agt_1";a.agentInstanceId="agi_1";
    a.backendGeneration=7;a.commandType="vdr.native.probe";a.payloadVersion=1;
    a.payload="{\"probeSchema\":1,\"probeNonce\":\"pbn_1\"}";
    a.verificationPolicy="readback_required";a.assignedAt=1;a.deadline=4102444800LL;
    a.requestFingerprint=backendAgentCommandFingerprint(a);
    assert(backendAgentCommandValidAssignment(a));
    return a;
}

std::string evidence(const std::string& receipt,bool readback)
{
    std::string value="{\"commandId\":\"cmd_1\",\"requestFingerprint\":\""+
        makeAssignment().requestFingerprint+"\",\"nativeOperation\":\"vdr.native.probe\",\"nativeOperationSchema\":1,\"pluginInstanceEpoch\":\"pie_1\",\"nativeExecutionSequence\":1,\"receiptCategory\":\""+receipt+"\",\"acceptedAt\":10,\"sideEffectClass\":\"none\",\"resultCategory\":\"succeeded\",\"vdrActive\":true,\"mutationsState\":\"disabled\",\"sideEffectObserved\":false,\"boundedDiagnostics\":\"native probe completed\",\"completedAt\":11";
    if(readback)value+=",\"readbackCategory\":\"verified\",\"duplicateDisposition\":\"exact_replay\"";
    return value+"}";
}

class Control final:public IBackendAgentControlPlaneTransport
{
public:
    BackendAgentCommandAssignment assignment=makeAssignment();
    int pollCalls=0,receiptCalls=0,resultCalls=0;
    std::string lastResult;
    BackendAgentTransportResponse postEnrollment(const std::string&,const std::string&,const std::string&,const std::string&) override{return {};}
    BackendAgentTransportResponse postAuthenticated(const std::string&,const std::string&,const std::string& path,const std::string& body) override
    {
        BackendAgentTransportResponse r;r.transportSucceeded=true;r.statusCode=200;
        if(path.find("/poll")!=std::string::npos){++pollCalls;BackendAgentCommandPollResult result;result.accepted=true;result.reasonCode="command_assigned";result.assignment=assignment;r.body=serializeBackendAgentCommandPollResponseJson(result);}
        else if(path.find("/receipt")!=std::string::npos){++receiptCalls;r.body="{}";}
        else if(path.find("/result")!=std::string::npos){++resultCalls;lastResult=body;r.body="{}";}
        return r;
    }
};

class Native final:public IBackendAgentNativeProbeTransport
{
public:
    int discoverCalls=0,executeCalls=0,readCalls=0;
    std::string epoch="pie_1";
    int executeFailuresRemaining=0;
    bool executionRecorded=false;
    SuiteBridgeCommandReply discoverNativeProbe() override
    {
        ++discoverCalls;return {SuiteBridgeTransportStatus::Success,900,"{\"nativeOperation\":\"vdr.native.probe\",\"nativeOperationSchema\":1,\"sideEffectClass\":\"none\",\"mutations\":\"disabled\",\"localProviderKind\":\"suitebridge\",\"pluginInstanceEpoch\":\""+epoch+"\"}",""};
    }
    SuiteBridgeCommandReply executeNativeProbe(const SuiteBridgeNativeProbeRequest&) override
    {
        ++executeCalls;
        if(executeFailuresRemaining>0){--executeFailuresRemaining;executionRecorded=true;return {SuiteBridgeTransportStatus::Timeout,0,"","timeout"};}
        const std::string category=executionRecorded?"duplicate":"accepted";
        executionRecorded=true;
        return {SuiteBridgeTransportStatus::Success,900,evidence(category,false),""};
    }
    SuiteBridgeCommandReply readNativeProbe(const SuiteBridgeNativeProbeReadbackRequest&) override
    {
        ++readCalls;return {SuiteBridgeTransportStatus::Success,900,evidence("duplicate",true),""};
    }
};
}

int main()
{
    const std::string path="/tmp/vdr-suite-native-command-state";
    std::remove(path.c_str());
    Control control;Native native;
    BackendAgentCommandClientConfig config{path,{"vdr.native.probe"},&native};
    BackendAgentCommandClientContext context{"agt_1","secret-material-at-least-thirty-two-bytes","default","agi_1",7};
    std::string reason;

    // The shared HTTP protocol parser must accept the explicitly negotiated
    // native probe type while continuing to reject arbitrary command names.
    BackendAgentCommandPollRequest nativePoll;
    nativePoll.backendId="default";
    nativePoll.agentInstanceId="agi_1";
    nativePoll.backendGeneration=7;
    nativePoll.supportedCommandTypes={"vdr.native.probe"};

    const std::string nativePollJson=
        serializeBackendAgentCommandPollRequestJson(nativePoll);

    BackendAgentCommandPollRequest parsedNativePoll;
    assert(parseBackendAgentCommandPollRequestJson(
        nativePollJson,parsedNativePoll,reason));
    assert(reason=="command_poll_parsed");
    assert(parsedNativePoll.supportedCommandTypes.size()==1);
    assert(
        parsedNativePoll.supportedCommandTypes.front()==
        "vdr.native.probe");

    BackendAgentCommandPollRequest unsupportedPoll=nativePoll;
    unsupportedPoll.supportedCommandTypes={"vdr.native.mutate"};

    BackendAgentCommandPollRequest rejectedPoll;
    assert(!parseBackendAgentCommandPollRequestJson(
        serializeBackendAgentCommandPollRequestJson(unsupportedPoll),
        rejectedPoll,
        reason));
    assert(reason=="invalid_command_poll_payload");

    assert(pollBackendAgentCommand(config,context,control,reason));
    assert(control.receiptCalls==1&&control.resultCalls==1);
    assert(native.executeCalls==1&&native.readCalls==1);
    assert(control.lastResult.find("\"verificationState\":\"verified\"")!=std::string::npos);
    std::ifstream in(path);std::string state((std::istreambuf_iterator<char>(in)),{});
    assert(state.find("dispatch_state=effect_reported")!=std::string::npos);
    assert(state.find("native_receipt_evidence={")!=std::string::npos);
    assert(state.find("native_result_evidence={")!=std::string::npos);
    assert(state.find("native_readback_evidence={")!=std::string::npos);

    // Control-plane replay returns persisted result and never executes natively again.
    assert(pollBackendAgentCommand(config,context,control,reason));
    assert(native.executeCalls==1&&native.readCalls==1);

    std::remove(path.c_str());
    Control recoveredControl;Native recoveredNative;recoveredNative.executeFailuresRemaining=1;
    BackendAgentCommandClientConfig recovered{path,{"vdr.native.probe"},&recoveredNative};
    assert(!pollBackendAgentCommand(recovered,context,recoveredControl,reason));
    assert(reason=="native_probe_dispatch_reconciliation_required");
    assert(recoveredControl.resultCalls==0&&recoveredNative.executeCalls==1);
    assert(reconcileBackendAgentCommandState(recovered,context,recoveredControl,reason));
    assert(recoveredNative.executeCalls==2&&recoveredNative.readCalls==1);
    assert(recoveredControl.lastResult.find("\"verificationState\":\"verified\"")!=std::string::npos);

    // New plugin epoch fences ambiguous starting state; no blind replay.
    std::remove(path.c_str());
    Control uncertainControl;Native uncertainNative;uncertainNative.executeFailuresRemaining=1;
    BackendAgentCommandClientConfig uncertain{path,{"vdr.native.probe"},&uncertainNative};
    assert(!pollBackendAgentCommand(uncertain,context,uncertainControl,reason));
    assert(uncertainNative.executeCalls==1);
    uncertainNative.epoch="pie_2";
    assert(reconcileBackendAgentCommandState(uncertain,context,uncertainControl,reason));
    assert(uncertainNative.executeCalls==1);
    assert(uncertainControl.lastResult.find("\"resultCategory\":\"outcome_unknown\"")!=std::string::npos);
    std::remove(path.c_str());
    return 0;
}

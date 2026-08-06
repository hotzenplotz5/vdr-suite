#include "BackendAgentCommandClient.h"

#include "BackendAgentClient.h"
#include "BackendAgentCommandJson.h"

#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
constexpr std::size_t MaximumStateBytes=64U*1024U;
std::int64_t nowSeconds(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
bool syncParent(const std::string& path){const auto p=path.find_last_of('/');const std::string parent=p==std::string::npos?".":p==0?"/":path.substr(0,p);const int fd=open(parent.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(fd<0)return false;const bool ok=fsync(fd)==0;const bool closed=close(fd)==0;return ok&&closed;}
bool writeAll(int fd,const std::string& value){std::size_t offset=0;while(offset<value.size()){const ssize_t n=write(fd,value.data()+offset,value.size()-offset);if(n<0){if(errno==EINTR)continue;return false;}if(n==0)return false;offset+=static_cast<std::size_t>(n);}return true;}
bool writeProtected(const std::string& path,const std::string& value,std::string& reason)
{
    if(path.empty()||path.front()!='/'||value.size()>MaximumStateBytes){reason="invalid_command_state_path";return false;}
    const std::string temp=path+".tmp"; const int fd=open(temp.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW,0600);
    if(fd<0){reason="command_state_open_failed";return false;} const bool ok=fchmod(fd,0600)==0&&writeAll(fd,value)&&fsync(fd)==0&&close(fd)==0&&rename(temp.c_str(),path.c_str())==0&&syncParent(path);
    if(!ok){close(fd);unlink(temp.c_str());reason="command_state_persist_failed";return false;} reason="command_state_persisted";return true;
}
bool retireProtectedState(const std::string& path,std::string& reason)
{
    if(path.empty()||path.front()!='/'){reason="invalid_command_state_path";return false;}
    struct stat status{};
    if(lstat(path.c_str(),&status)!=0){reason=errno==ENOENT?"command_state_not_found":"command_state_stat_failed";return errno==ENOENT;}
    if(!S_ISREG(status.st_mode)||(status.st_mode&(S_IRWXG|S_IRWXO))!=0){reason="command_state_unprotected";return false;}
    if(unlink(path.c_str())!=0||!syncParent(path)){reason="command_state_retire_failed";return false;}
    reason="completed_command_state_retired";
    return true;
}
bool readStateFile(const std::string& path,std::map<std::string,std::string>& values,std::string& reason)
{
    values.clear();struct stat st{};if(lstat(path.c_str(),&st)!=0){reason=errno==ENOENT?"command_state_not_found":"command_state_stat_failed";return false;}
    if(!S_ISREG(st.st_mode)||(st.st_mode&(S_IRWXG|S_IRWXO))!=0||st.st_size<0||static_cast<std::size_t>(st.st_size)>MaximumStateBytes){reason="command_state_unprotected";return false;}
    std::ifstream in(path);if(!in){reason="command_state_open_failed";return false;}std::string line;while(std::getline(in,line)){const auto pos=line.find('=');if(pos==std::string::npos||pos==0||values.count(line.substr(0,pos))){reason="command_state_invalid";return false;}values[line.substr(0,pos)]=line.substr(pos+1);}return true;
}
bool number(const std::string& value,std::uint64_t& out){if(value.empty()||value.size()>19)return false;out=0;for(unsigned char c:value){if(c<'0'||c>'9')return false;const unsigned d=c-'0';if(out>(static_cast<std::uint64_t>(INT64_MAX)-d)/10)return false;out=out*10+d;}return true;}
struct LocalState
{
    BackendAgentCommandAssignment assignment;
    BackendAgentCommandReceipt receipt;
    BackendAgentCommandResult result;
    bool receiptAcknowledged=false;
    bool resultPresent=false;
    bool resultAcknowledged=false;
    std::string dispatchState="not_started";
};
std::string boolText(bool value){return value?"1":"0";}
bool load(const std::string& path,LocalState& state,std::string& reason)
{
    std::map<std::string,std::string> v;if(!readStateFile(path,v,reason))return false;
    static const std::vector<std::string> keys={"version","protocol_version","request_id","correlation_id","operation_id","job_id","attempt_id","claim_epoch","command_id","backend_id","agent_id","agent_instance_id","backend_generation","command_type","payload_version","payload","request_fingerprint","verification_policy","assigned_at","deadline","receipt_category","received_at","receipt_reason","receipt_acknowledged","dispatch_state","result_present","result_acknowledged","verification_state","result_category","error_category","retry_classification","bounded_diagnostics","completed_at"};
    if(v.size()!=keys.size()){reason="command_state_invalid";return false;}for(const auto& key:keys)if(!v.count(key)){reason="command_state_invalid";return false;}
    std::uint64_t claim=0,generation=0,payloadVersion=0,assigned=0,deadline=0,received=0,completed=0;
    if(v["version"]!="1"||!number(v["claim_epoch"],claim)||!number(v["backend_generation"],generation)||!number(v["payload_version"],payloadVersion)||!number(v["assigned_at"],assigned)||!number(v["deadline"],deadline)||!number(v["received_at"],received)||!number(v["completed_at"],completed)){reason="command_state_invalid";return false;}
    auto& a=state.assignment;a.present=true;a.protocolVersion=v["protocol_version"];a.requestId=v["request_id"];a.correlationId=v["correlation_id"];a.operationId=v["operation_id"];a.jobId=v["job_id"];a.attemptId=v["attempt_id"];a.claimEpoch=claim;a.commandId=v["command_id"];a.backendId=v["backend_id"];a.agentId=v["agent_id"];a.agentInstanceId=v["agent_instance_id"];a.backendGeneration=generation;a.commandType=v["command_type"];a.payloadVersion=payloadVersion;a.payload=v["payload"];a.requestFingerprint=v["request_fingerprint"];a.verificationPolicy=v["verification_policy"];a.assignedAt=static_cast<std::int64_t>(assigned);a.deadline=static_cast<std::int64_t>(deadline);
    if(!backendAgentCommandValidAssignment(a)){reason="command_state_invalid_assignment";return false;}
    auto& r=state.receipt;r.commandId=a.commandId;r.requestFingerprint=a.requestFingerprint;r.jobId=a.jobId;r.attemptId=a.attemptId;r.claimEpoch=a.claimEpoch;r.backendId=a.backendId;r.agentId=a.agentId;r.agentInstanceId=a.agentInstanceId;r.backendGeneration=a.backendGeneration;r.receiptCategory=v["receipt_category"];r.receivedAt=static_cast<std::int64_t>(received);r.reasonCode=v["receipt_reason"];
    state.receiptAcknowledged=v["receipt_acknowledged"]=="1";state.dispatchState=v["dispatch_state"];state.resultPresent=v["result_present"]=="1";state.resultAcknowledged=v["result_acknowledged"]=="1";
    if(state.resultPresent){auto& x=state.result;x.commandId=a.commandId;x.requestFingerprint=a.requestFingerprint;x.jobId=a.jobId;x.attemptId=a.attemptId;x.claimEpoch=a.claimEpoch;x.backendId=a.backendId;x.agentId=a.agentId;x.agentInstanceId=a.agentInstanceId;x.backendGeneration=a.backendGeneration;x.dispatchState=state.dispatchState;x.verificationState=v["verification_state"];x.resultCategory=v["result_category"];x.errorCategory=v["error_category"];x.retryClassification=v["retry_classification"];x.boundedDiagnostics=v["bounded_diagnostics"];x.completedAt=static_cast<std::int64_t>(completed);if(!backendAgentCommandValidResult(x)){reason="command_state_invalid_result";return false;}}
    reason="command_state_loaded";return true;
}
bool persist(const std::string& path,const LocalState& state,std::string& reason)
{
    const auto& a=state.assignment;const auto& r=state.receipt;const auto& x=state.result;std::ostringstream out;
    out<<"version=1\nprotocol_version="<<a.protocolVersion<<"\nrequest_id="<<a.requestId<<"\ncorrelation_id="<<a.correlationId<<"\noperation_id="<<a.operationId<<"\njob_id="<<a.jobId<<"\nattempt_id="<<a.attemptId<<"\nclaim_epoch="<<a.claimEpoch<<"\ncommand_id="<<a.commandId<<"\nbackend_id="<<a.backendId<<"\nagent_id="<<a.agentId<<"\nagent_instance_id="<<a.agentInstanceId<<"\nbackend_generation="<<a.backendGeneration<<"\ncommand_type="<<a.commandType<<"\npayload_version="<<a.payloadVersion<<"\npayload="<<a.payload<<"\nrequest_fingerprint="<<a.requestFingerprint<<"\nverification_policy="<<a.verificationPolicy<<"\nassigned_at="<<a.assignedAt<<"\ndeadline="<<a.deadline<<"\nreceipt_category="<<r.receiptCategory<<"\nreceived_at="<<r.receivedAt<<"\nreceipt_reason="<<r.reasonCode<<"\nreceipt_acknowledged="<<boolText(state.receiptAcknowledged)<<"\ndispatch_state="<<state.dispatchState<<"\nresult_present="<<boolText(state.resultPresent)<<"\nresult_acknowledged="<<boolText(state.resultAcknowledged)<<"\nverification_state="<<(state.resultPresent?x.verificationState:"")<<"\nresult_category="<<(state.resultPresent?x.resultCategory:"")<<"\nerror_category="<<(state.resultPresent?x.errorCategory:"")<<"\nretry_classification="<<(state.resultPresent?x.retryClassification:"")<<"\nbounded_diagnostics="<<(state.resultPresent?x.boundedDiagnostics:"")<<"\ncompleted_at="<<(state.resultPresent?x.completedAt:0)<<"\n";
    return writeProtected(path,out.str(),reason);
}
bool sameContext(const BackendAgentCommandAssignment& a,const BackendAgentCommandClientContext& c){return a.backendId==c.backendId&&a.agentId==c.agentId&&a.agentInstanceId==c.agentInstanceId&&a.backendGeneration==c.backendGeneration;}
std::string responseCode(const BackendAgentTransportResponse& response){return response.errorCode.empty()?"command_transport_failed":response.errorCode;}

bool sendReceipt(const BackendAgentCommandClientConfig& config,const BackendAgentCommandClientContext& context,IBackendAgentControlPlaneTransport& transport,LocalState& state,std::string& reason)
{
    const auto response=transport.postAuthenticated(context.agentId,context.credentialSecret,"/api/agent/v1/commands/receipt",serializeBackendAgentCommandReceiptJson(state.receipt));
    if(!response.transportSucceeded||response.statusCode!=200){reason=responseCode(response);return false;}state.receiptAcknowledged=true;return persist(config.statePath,state,reason);
}
bool sendResult(const BackendAgentCommandClientConfig& config,const BackendAgentCommandClientContext& context,IBackendAgentControlPlaneTransport& transport,LocalState& state,std::string& reason)
{
    const auto response=transport.postAuthenticated(context.agentId,context.credentialSecret,"/api/agent/v1/commands/result",serializeBackendAgentCommandResultJson(state.result));
    if(!response.transportSucceeded||response.statusCode!=200){reason=responseCode(response);return false;}state.resultAcknowledged=true;return persist(config.statePath,state,reason);
}
void createResult(LocalState& state,const std::string& dispatch,const std::string& verification,const std::string& category,const std::string& error,const std::string& retry,const std::string& diagnostics)
{
    state.dispatchState=dispatch;state.resultPresent=true;state.resultAcknowledged=false;auto& x=state.result;const auto& a=state.assignment;x.commandId=a.commandId;x.requestFingerprint=a.requestFingerprint;x.jobId=a.jobId;x.attemptId=a.attemptId;x.claimEpoch=a.claimEpoch;x.backendId=a.backendId;x.agentId=a.agentId;x.agentInstanceId=a.agentInstanceId;x.backendGeneration=a.backendGeneration;x.dispatchState=dispatch;x.verificationState=verification;x.resultCategory=category;x.errorCategory=error;x.retryClassification=retry;x.boundedDiagnostics=diagnostics;x.completedAt=nowSeconds();
}
}

bool reconcileBackendAgentCommandState(const BackendAgentCommandClientConfig& config,const BackendAgentCommandClientContext& context,IBackendAgentControlPlaneTransport& transport,std::string& reason)
{
    if(config.commandTypes.empty()){reason="command_delivery_disabled";return true;}LocalState state;if(!load(config.statePath,state,reason)){if(reason=="command_state_not_found"){reason="no_local_command";return true;}return false;}
    if(!sameContext(state.assignment,context))
    {
        if(state.resultPresent&&state.receiptAcknowledged&&state.resultAcknowledged)
            return retireProtectedState(config.statePath,reason);
        reason="local_command_generation_fenced";
        return false;
    }
    if(!state.receiptAcknowledged&&!sendReceipt(config,context,transport,state,reason))return false;
    if(state.resultPresent){if(!state.resultAcknowledged&&!sendResult(config,context,transport,state,reason))return false;reason="command_result_reconciled";return true;}
    if(state.dispatchState=="starting"||state.dispatchState=="accepted_by_executor")
    {
        createResult(state,state.dispatchState,"outcome_unknown","outcome_unknown","executor_unknown","reconcile_only","probe execution boundary recovered without re-execution");if(!persist(config.statePath,state,reason))return false;return sendResult(config,context,transport,state,reason);
    }
    if(state.dispatchState!="not_started"||state.assignment.commandType!="probe.noop"){reason="unsupported_local_command_state";return false;}
    state.dispatchState="starting";if(!persist(config.statePath,state,reason))return false;
    state.dispatchState="accepted_by_executor";if(!persist(config.statePath,state,reason))return false;
    createResult(state,"effect_reported","not_required","succeeded","none","none","probe.noop completed without native side effect");if(!persist(config.statePath,state,reason))return false;return sendResult(config,context,transport,state,reason);
}

bool pollBackendAgentCommand(const BackendAgentCommandClientConfig& config,const BackendAgentCommandClientContext& context,IBackendAgentControlPlaneTransport& transport,std::string& reason)
{
    if(config.commandTypes.empty()){reason="command_delivery_disabled";return true;}
    if(!reconcileBackendAgentCommandState(config,context,transport,reason)&&reason!="no_local_command")return false;
    BackendAgentCommandPollRequest request;request.backendId=context.backendId;request.agentInstanceId=context.agentInstanceId;request.backendGeneration=context.backendGeneration;request.supportedCommandTypes=config.commandTypes;
    const auto response=transport.postAuthenticated(context.agentId,context.credentialSecret,"/api/agent/v1/commands/poll",serializeBackendAgentCommandPollRequestJson(request));
    if(!response.transportSucceeded||response.statusCode!=200){reason=responseCode(response);return false;}BackendAgentCommandPollResult result;if(!parseBackendAgentCommandPollResponseJson(response.body,result,reason))return false;if(!result.assignment.present){reason="no_command_available";return true;}if(!sameContext(result.assignment,context)){reason="command_assignment_context_mismatch";return false;}
    LocalState current;std::string loadReason;if(load(config.statePath,current,loadReason))
    {
        if(current.assignment.commandId==result.assignment.commandId){if(current.assignment.requestFingerprint!=result.assignment.requestFingerprint){reason="conflicting_duplicate_command";return false;}current.receiptAcknowledged=false;if(current.resultPresent)current.resultAcknowledged=false;if(!persist(config.statePath,current,reason))return false;return reconcileBackendAgentCommandState(config,context,transport,reason);}
        if(!current.resultAcknowledged){reason="local_command_inbox_busy";return false;}
    }
    else if(loadReason!="command_state_not_found"){reason=loadReason;return false;}
    LocalState state;state.assignment=result.assignment;auto& receipt=state.receipt;receipt.commandId=result.assignment.commandId;receipt.requestFingerprint=result.assignment.requestFingerprint;receipt.jobId=result.assignment.jobId;receipt.attemptId=result.assignment.attemptId;receipt.claimEpoch=result.assignment.claimEpoch;receipt.backendId=result.assignment.backendId;receipt.agentId=result.assignment.agentId;receipt.agentInstanceId=result.assignment.agentInstanceId;receipt.backendGeneration=result.assignment.backendGeneration;receipt.receiptCategory="accepted";receipt.receivedAt=nowSeconds();receipt.reasonCode="durably_recorded";
    if (!persist(config.statePath, state, reason)) return false;
    return reconcileBackendAgentCommandState(config, context, transport, reason);
}

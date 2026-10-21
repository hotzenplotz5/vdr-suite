#include "BackendAgentMediaImportAuthorization.h"
#include "BackendAgentClient.h"
#include "BackendAgentCommandStateStore.h"
#include "MediaToolsAgentExecutor.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>
namespace fs = std::filesystem;
using namespace vdrsuite::agent;
namespace {
std::int64_t now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
BackendAgentCommandAssignment assignment() {
    BackendAgentCommandAssignment a;
    a.present=true; a.requestId="req_auth"; a.correlationId="corr_auth";
    a.operationId="op_auth"; a.jobId="job_auth"; a.attemptId="att_auth";
    a.claimEpoch=7; a.commandId="cmd_auth"; a.backendId="backend";
    a.agentId="agent"; a.agentInstanceId="instance"; a.backendGeneration=34;
    a.commandType=vdrsuite::mediatools::kMediaImportCommandType;
    a.payloadVersion=1; a.payload="{\"sourceId\":\"sourceKey_ABCD1234\"}";
    a.verificationPolicy="reconciliation_required";
    a.assignedAt=now()-1; a.deadline=now()+60;
    a.requestFingerprint=backendAgentCommandFingerprint(a);
    assert(backendAgentCommandValidAssignment(a));
    return a;
}
struct Transport final : IBackendAgentControlPlaneTransport {
    bool allowed=true; int checks=0;
    BackendAgentTransportResponse postEnrollment(
        const std::string&,const std::string&,const std::string&,
        const std::string&) override { return {}; }
    BackendAgentTransportResponse postAuthenticated(
        const std::string& agent,const std::string& secret,
        const std::string& path,const std::string& body) override {
        assert(agent=="agent" && secret=="secret");
        assert(path=="/api/agent/v1/commands/receipt");
        assert(body.find("\"commandId\":\"cmd_auth\"")!=std::string::npos);
        ++checks;
        if (!allowed) return {true,409,"","media_import_permission_denied"};
        return {true,200,"{\"outcome\":\"replayed\",\"reasonCode\":\"command_receipt_replayed\"}",""};
    }
};
struct Fixture {
    fs::path directory, path;
    commandstate::LocalState state;
    Fixture() {
        const char* temp=std::getenv("TMPDIR");
        if (!temp || !*temp) throw std::runtime_error("TMPDIR required");
        std::string pattern=std::string(temp)+"/a6-auth-XXXXXX";
        std::vector<char> name(pattern.begin(),pattern.end());
        name.push_back('\0');
        if (!::mkdtemp(name.data())) throw std::runtime_error("mkdtemp");
        directory=name.data(); path=directory/"command.state";
        state.assignment=assignment();
        const auto& a=state.assignment;
        auto& r=state.receipt;
        r.commandId=a.commandId; r.requestFingerprint=a.requestFingerprint;
        r.jobId=a.jobId; r.attemptId=a.attemptId; r.claimEpoch=a.claimEpoch;
        r.backendId=a.backendId; r.agentId=a.agentId;
        r.agentInstanceId=a.agentInstanceId;
        r.backendGeneration=a.backendGeneration;
        r.receiptCategory="accepted"; r.receivedAt=now();
        r.reasonCode="durably_recorded";
        state.receiptAcknowledged=true; persist();
    }
    void persist() {
        std::string reason;
        if (!commandstate::persist(path.string(),state,reason))
            throw std::runtime_error(reason);
    }
    ~Fixture() { std::error_code e; fs::remove_all(directory,e); }
};
}
int main() {
    Fixture f; Transport t; BackendAgentCommandClientContext ctx;
    ctx.backendId="backend";ctx.agentId="agent";
    ctx.agentInstanceId="instance";ctx.backendGeneration=34;
    ctx.credentialSecret="secret";
    ReceiptRevalidatingMediaImportAuthorization auth(
        f.path.string(),t,[&]{ return ctx; });
    auto a=f.state.assignment;
    assert(auth.granted(a));
    assert(t.checks==1);
    t.allowed=false; assert(!auth.granted(a)); t.allowed=true;
    f.state.dispatchState="starting";f.persist();assert(auth.granted(a));
    f.state.resultPresent=true;f.persist();assert(!auth.granted(a));
    f.state.resultPresent=false;f.persist();
    ctx.backendGeneration=35;assert(!auth.granted(a));
    ctx.backendGeneration=34;
    a.claimEpoch=8;a.requestFingerprint=backendAgentCommandFingerprint(a);
    assert(!auth.granted(a));
    a=f.state.assignment;
    f.state.receiptAcknowledged=false;f.persist();assert(!auth.granted(a));
    f.state.receiptAcknowledged=true;
    f.state.receipt.reasonCode="forged";f.persist();assert(!auth.granted(a));
    f.state.receipt.reasonCode="durably_recorded";f.persist();
    assert(auth.granted(a));
}

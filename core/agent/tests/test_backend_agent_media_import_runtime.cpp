#include "BackendAgentMediaImportRuntime.h"
#include "BackendAgentClient.h"
#include "BackendAgentCommandClient.h"
#include "BackendAgentCommandStateStore.h"
#include "BackendAgentCommandJson.h"
#include "MediaToolsAgentExecutor.h"
#include "MediaToolsImportRunLock.h"
#include <openssl/evp.h>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace vdrsuite::agent;

namespace {
std::int64_t now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
std::string fileDigest(const fs::path& path) {
    std::ifstream in(path,std::ios::binary);
    if (!in) throw std::runtime_error("fixture binary open");
    EVP_MD_CTX* ctx=EVP_MD_CTX_new();
    if (!ctx || EVP_DigestInit_ex(ctx,EVP_sha256(),nullptr)!=1)
        throw std::runtime_error("fixture digest init");
    char bytes[8192]; unsigned char digest[EVP_MAX_MD_SIZE];unsigned n=0;
    while(in) {
        in.read(bytes,sizeof bytes);
        if (in.gcount()>0 &&
            EVP_DigestUpdate(ctx,bytes,static_cast<std::size_t>(in.gcount()))!=1)
            throw std::runtime_error("fixture digest update");
    }
    if(EVP_DigestFinal_ex(ctx,digest,&n)!=1||n!=32)
        throw std::runtime_error("fixture digest complete");
    EVP_MD_CTX_free(ctx);
    const char* hex="0123456789abcdef";
    std::string result;
    for(unsigned i=0;i<n;++i) {
        result+=hex[digest[i]>>4];
        result+=hex[digest[i]&15];
    }
    return result;
}
BackendAgentCommandAssignment command() {
    BackendAgentCommandAssignment a;
    a.present=true;a.requestId="request_A6";a.correlationId="corr_A6";
    a.operationId="operation_A6";a.jobId="job_A6";a.attemptId="attempt_A6";
    a.claimEpoch=4;a.commandId="command_A6";a.backendId="backend";
    a.agentId="agent";a.agentInstanceId="instance";a.backendGeneration=19;
    a.commandType=vdrsuite::mediatools::kMediaImportCommandType;
    a.payloadVersion=1;a.payload="{\"sourceId\":\"inventory_Token_0001\"}";
    a.verificationPolicy="reconciliation_required";
    a.assignedAt=now()-1;a.deadline=now()+60;
    a.requestFingerprint=backendAgentCommandFingerprint(a);
    assert(backendAgentCommandValidAssignment(a));return a;
}
struct Transport final: IBackendAgentControlPlaneTransport {
    bool allowed=true;
    bool firstReceiptAccepted=false;
    unsigned checks=0;
    unsigned resultAcks=0;
    BackendAgentTransportResponse postEnrollment(
        const std::string&,const std::string&,const std::string&,
        const std::string&) override { return {}; }
    BackendAgentTransportResponse postAuthenticated(
        const std::string& id,const std::string& secret,
        const std::string& path,const std::string& body) override {
        assert(id=="agent" && secret=="secret");
        if(path=="/api/agent/v1/commands/result") {
            ++resultAcks;
            BackendAgentCommandResult result;
            std::string reason;
            assert(parseBackendAgentCommandResultJson(body,result,reason));
            assert(result.commandId=="command_A6");
            assert(result.verificationState=="outcome_unknown");
            return {true,200,"{}",""};
        }
        assert(path=="/api/agent/v1/commands/receipt");
        assert(body.find("\"commandId\":\"command_A6\"")!=std::string::npos);
        ++checks;
        if(firstReceiptAccepted && checks==1) return {true,200,
            "{\"outcome\":\"accepted\",\"reasonCode\":\"command_receipt_accepted\"}",""};
        if(!allowed) return {true,409,"","media_import_permission_denied"};
        return {true,200,
            "{\"outcome\":\"replayed\",\"reasonCode\":\"command_receipt_replayed\"}",""};
    }
};
struct Fixture {
    fs::path root,binary,approval,manifest,runLock,ledger,source,video,statePath;
    Fixture() {
        if(::geteuid()!=0 || !std::getenv("TMPDIR") ||
           std::string(std::getenv("TMPDIR"))!="/root")
            throw std::runtime_error("CI-only root fixture required");
        struct stat parent{};
        if(::stat("/run",&parent)!=0||!S_ISDIR(parent.st_mode)||
           parent.st_uid!=0||(parent.st_mode&0022))
            throw std::runtime_error("unsafe CI fixture parent");
        std::string pattern="/run/vdr-a6-runtime-XXXXXX";
        std::vector<char> name(pattern.begin(),pattern.end());
        name.push_back('\0');
        if(!::mkdtemp(name.data()))throw std::runtime_error("mkdtemp");
        root=name.data();
        binary=root/"usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import";
        approval=root/"etc/vdr-suite/addon-approvals/rectools.json";
        // The Agent verifies the root-owned companion beside its pinned handler.
        manifest=root/"usr/libexec/vdr-suite/addons/rectools/addon.json";
        runLock=approval.parent_path()/vdrsuite::mediatools::kMediaImportRunLockName;
        ledger=root/"etc/vdr-suite/addon-sources/media-import.tsv";
        source=root/"media/in/movie.ts";
        video=root/"media/out";
        statePath=root/"state/command.state";
        fs::create_directories(binary.parent_path());
        fs::create_directories(approval.parent_path());
        fs::create_directories(manifest.parent_path());
        // Match the separate descriptive metadata tree in the composed Stage.
        fs::create_directories(root/"usr/share/vdr-suite/addons/rectools");
        fs::create_directories(ledger.parent_path());
        fs::create_directories(source.parent_path());
        fs::create_directories(video);
        fs::create_directories(statePath.parent_path());
        for(const char* sub : {"", "usr", "usr/libexec",
            "usr/libexec/vdr-suite", "usr/libexec/vdr-suite/addons",
            "usr/libexec/vdr-suite/addons/rectools", "etc",
            "etc/vdr-suite","etc/vdr-suite/addon-approvals",
            "etc/vdr-suite/addon-sources", "usr/share",
            "usr/share/vdr-suite", "usr/share/vdr-suite/addons",
            "usr/share/vdr-suite/addons/rectools",
            "media", "media/in", "media/out",
            "state"})
            if(::chmod((root/sub).c_str(),0755)!=0)
                throw std::runtime_error("fixture mkdir mode");
        // The synthetic executable is the actual /bin/true ELF. It cannot
        // create recordings, even when the pinned fexecve path is exercised.
        fs::copy_file("/bin/true",binary);
        if(::chmod(binary.c_str(),0755)!=0)
            throw std::runtime_error("binary chmod");
        writeManifest();
        writeApproval(false);
        {std::ofstream file(runLock,std::ios::binary|std::ios::trunc);}
        if(::chmod(runLock.c_str(),0644)!=0)
            throw std::runtime_error("run lock chmod");
        {std::ofstream file(source);file<<"synthetic-media-input";}
        {std::ofstream file(ledger);
         file<<"backend\tinventory_Token_0001\t"
             <<(root/"media/in").string()<<"\t"<<video.string()
             <<"\t"<<source.string()<<"\n";}
        if(::chmod(ledger.c_str(),0644)!=0)
            throw std::runtime_error("ledger chmod");
        commandstate::LocalState state;
        state.assignment=command();
        const auto& a=state.assignment;
        auto& r=state.receipt;
        r.commandId=a.commandId;r.requestFingerprint=a.requestFingerprint;
        r.jobId=a.jobId;r.attemptId=a.attemptId;r.claimEpoch=a.claimEpoch;
        r.backendId=a.backendId;r.agentId=a.agentId;
        r.agentInstanceId=a.agentInstanceId;
        r.backendGeneration=a.backendGeneration;
        r.receiptCategory="accepted";r.receivedAt=now();
        r.reasonCode="durably_recorded";
        state.receiptAcknowledged=true;
        std::string reason;
        if(!commandstate::persist(statePath.string(),state,reason))
            throw std::runtime_error(reason);
    }
    void writeManifest() {
        std::ofstream out(manifest, std::ios::trunc);
        out << "{\"schemaVersion\":1,\"id\":\"rectools\",\"version\":\"0.1.0\","
               "\"package\":\"vdr-suite-addon-media-tools\",\"state\":\"scaffold\","
               "\"displayName\":\"Media Tools\",\"description\":\"Inactive\","
               "\"entrypoints\":[],\"capabilities\":[],\"permissions\":[],"
               "\"dependencies\":[]}\n";
        out.close();
        if (::chmod(manifest.c_str(),0644)!=0)
            throw std::runtime_error("manifest chmod");
    }
    void writeApproval(bool revoked) {
        {std::ofstream out(approval);
         out<<"{\"id\":\"rectools\",\"origin\":\"manual\","
               "\"package\":\"vdr-suite-addon-media-tools\","
               "\"schemaVersion\":1,\"sha256\":\""
            <<fileDigest(binary)<<"\",\"state\":\""
            <<(revoked?"revoked":"approved")
            <<"\",\"version\":\"0.1.0\"}\n";}
        if(::chmod(approval.c_str(),0644)!=0)
            throw std::runtime_error("approval chmod");
    }
    ~Fixture(){std::error_code e;fs::remove_all(root,e);}
};
}

void verifyDurableAgentLifecycle()
{
    const BackendAgentCommandClientContext current{
        "agent", "secret", "backend", "instance", 19};

    const auto runFromUnacknowledgedReceipt = [&](Fixture& fixture,
        Transport& server, BackendAgentMediaImportRuntime& executor) {
        BackendAgentCommandClientConfig configuration;
        configuration.statePath = fixture.statePath.string();
        configuration.commandTypes = {
            vdrsuite::mediatools::kMediaImportCommandType};
        configuration.mediaImportExecutor = &executor;
        std::string reason;
        commandstate::LocalState pending;
        assert(commandstate::load(configuration.statePath, pending, reason));
        pending.receiptAcknowledged = false;
        assert(commandstate::persist(configuration.statePath, pending, reason));
        assert(reconcileBackendAgentCommandState(
            configuration, current, server, reason));
        commandstate::LocalState completed;
        assert(commandstate::load(
            configuration.statePath, completed, reason));
        assert(completed.receiptAcknowledged && completed.resultPresent &&
               completed.resultAcknowledged && server.resultAcks == 1);
        const unsigned checks = server.checks;
        assert(reconcileBackendAgentCommandState(
            configuration, current, server, reason));
        assert(server.resultAcks == 1 && server.checks == checks);
        return completed;
    };

    // One actual protected Agent inbox, two authoritative receipt rechecks,
    // one actual pinned /bin/true child, durable result and restart replay.
    Fixture normal;
    Transport allowed;
    allowed.firstReceiptAccepted = true;
    BackendAgentMediaImportRuntime executor(
        normal.root.string(), "backend", normal.statePath.string(), allowed,
        [&] { return current; });
    const auto finished = runFromUnacknowledgedReceipt(
        normal, allowed, executor);
    assert(allowed.checks == 3 && allowed.resultAcks == 1);
    assert(finished.dispatchState == "accepted_by_executor");
    assert(finished.result.resultCategory == "outcome_unknown");
    assert(finished.result.retryClassification == "reconcile_only");
    assert(finished.result.resultEvidence.empty());
    assert(fs::is_empty(normal.video));

    // The server accepted receipt, then withdrew effective permission.
    // The executable must not start, even though it is installed and pinned.
    Fixture revoked;
    Transport blocked;
    blocked.firstReceiptAccepted = true;
    blocked.allowed = false;
    BackendAgentMediaImportRuntime revocationGuard(
        revoked.root.string(), "backend", revoked.statePath.string(), blocked,
        [&] { return current; });
    const auto denied = runFromUnacknowledgedReceipt(
        revoked, blocked, revocationGuard);
    assert(blocked.checks == 2 && blocked.resultAcks == 1);
    assert(denied.result.resultCategory == "rejected");
    assert(denied.dispatchState == "not_started");
    assert(fs::is_empty(revoked.video));

    // Restart from durable 'starting' cannot attempt the import again.
    Fixture crashed;
    Transport crashServer;
    commandstate::LocalState inFlight;
    std::string reason;
    assert(commandstate::load(
        crashed.statePath.string(), inFlight, reason));
    inFlight.dispatchState = "starting";
    assert(commandstate::persist(
        crashed.statePath.string(), inFlight, reason));
    BackendAgentMediaImportRuntime crashedRuntime(
        crashed.root.string(), "backend", crashed.statePath.string(), crashServer,
        [&] { return current; });
    BackendAgentCommandClientConfig crashConfig;
    crashConfig.statePath = crashed.statePath.string();
    crashConfig.commandTypes = {
        vdrsuite::mediatools::kMediaImportCommandType};
    crashConfig.mediaImportExecutor = &crashedRuntime;
    assert(reconcileBackendAgentCommandState(
        crashConfig, current, crashServer, reason));
    assert(crashServer.checks == 0 && crashServer.resultAcks == 1);
    assert(commandstate::load(
        crashConfig.statePath, inFlight, reason));
    assert(inFlight.resultPresent &&
           inFlight.result.resultCategory == "outcome_unknown" &&
           inFlight.result.retryClassification == "reconcile_only");
    assert(reconcileBackendAgentCommandState(
        crashConfig, current, crashServer, reason));
    assert(crashServer.resultAcks == 1);
    assert(fs::is_empty(crashed.video));
}

int main() {
    Fixture f;
    Transport transport;
    BackendAgentCommandClientContext current;
    current.agentId="agent";current.credentialSecret="secret";
    current.backendId="backend";current.agentInstanceId="instance";
    current.backendGeneration=19;

    BackendAgentMediaImportRuntime runtime(
        f.root.string(),"backend",f.statePath.string(),transport,
        [&]{return current;});
    BackendAgentCommandClientConfig config;
    config.commandTypes={vdrsuite::mediatools::kMediaImportCommandType};
    // Core's default is still null; even a hand-populated command allowlist
    // does not advertise anything without a separately injected executor.
    assert(discoverBackendAgentCommandAvailability(config).commandTypes.empty());
    config.mediaImportExecutor=&runtime;
    assert(runtime.available());
    assert(discoverBackendAgentCommandAvailability(config).commandTypes.size()==1);

    // One observation reads the trusted inventory once and reuses its exact
    // selected descriptor for a read-only preview. This synthetic .ts file
    // is deliberately not a real codec stream; no import can be submitted.
    std::vector<std::string> observedIds;
    std::vector<vdrsuite::mediatools::MediaImportSourceLabel> observedLabels;
    vdrsuite::mediatools::MediaImportSourcePreview observedPreview;
    bool observedPartial = true;
    assert(runtime.listTrustedSourceObservation(
        observedIds, observedLabels, observedPartial,
        "inventory_Token_0001", observedPreview));
    assert(!observedPartial && observedIds.size() == 1 &&
           observedLabels.size() == 1 &&
           observedIds[0] == "inventory_Token_0001" &&
           observedLabels[0].sourceId == observedIds[0]);
    assert(observedPreview.sourceId == observedIds[0] &&
           observedPreview.status == "unavailable" &&
           observedPreview.failureCode == "ffprobe_failed");
    assert(runtime.listTrustedSourceObservation(
        observedIds, observedLabels, observedPartial,
        "absent_Token_00001", observedPreview));
    assert(observedIds.size() == 1 &&
           observedPreview.status == "unavailable" &&
           observedPreview.failureCode == "inventory_missing");

    fs::remove(f.runLock);
    assert(!runtime.available());
    assert(discoverBackendAgentCommandAvailability(config).commandTypes.empty());
    {std::ofstream lockFile(f.runLock,std::ios::trunc|std::ios::binary);}
    assert(::chmod(f.runLock.c_str(),0644)==0);
    const fs::path savedLock=f.runLock.string()+".original";
    fs::rename(f.runLock,savedLock);
    fs::create_symlink(savedLock.filename(),f.runLock);
    assert(!runtime.available());
    fs::remove(f.runLock);
    fs::rename(savedLock,f.runLock);
    auto exclusive=vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::exclusiveMaintenance);
    assert(exclusive.has_value());
    assert(!runtime.available());
    assert(discoverBackendAgentCommandAvailability(config).commandTypes.empty());
    exclusive.reset();
    assert(runtime.available());

    const auto a=command();
    assert(runtime.authorize(a,"backend",19));
    transport.allowed=false;
    assert(runtime.execute(a)==vdrsuite::mediatools::MediaImportAgentOutcome::rejected);
    transport.allowed=true;
    assert(runtime.authorize(a,"backend",19));
    assert(runtime.execute(a)==vdrsuite::mediatools::MediaImportAgentOutcome::submitted);
    assert(runtime.execute(a)==vdrsuite::mediatools::MediaImportAgentOutcome::rejected);
    assert(runtime.takeRecordingEvidence().empty());
    assert(fs::is_empty(f.video));

    // A valid permit becomes invalid if the server advances backend
    // generation AFTER authorization but BEFORE actual pinned execution.
    assert(runtime.authorize(a,"backend",19));
    current.backendGeneration=20;
    assert(runtime.execute(a)==vdrsuite::mediatools::MediaImportAgentOutcome::rejected);
    current.backendGeneration=19;
    assert(runtime.execute(a)==vdrsuite::mediatools::MediaImportAgentOutcome::rejected);

    // An Agent restart changes its instance identity without changing
    // the stored command or the locally pinned handler descriptor.
    assert(runtime.authorize(a,"backend",19));
    current.agentInstanceId="replacement";
    assert(runtime.execute(a)==vdrsuite::mediatools::MediaImportAgentOutcome::rejected);
    current.agentInstanceId="instance";
    assert(runtime.execute(a)==vdrsuite::mediatools::MediaImportAgentOutcome::rejected);

    current.backendGeneration=20;
    assert(!runtime.available() || !runtime.authorize(a,"backend",19));
    current.backendGeneration=19;
    f.writeApproval(true);
    assert(!runtime.available());
    assert(discoverBackendAgentCommandAvailability(config).commandTypes.empty());
    assert(!runtime.authorize(a,"backend",19));
    f.writeApproval(false);

    assert(::chmod(f.ledger.c_str(),0666)==0);
    assert(!runtime.available());
    assert(::chmod(f.ledger.c_str(),0644)==0);
    assert(runtime.available());
    fs::remove(f.manifest);
    assert(!runtime.available());
    assert(discoverBackendAgentCommandAvailability(config).commandTypes.empty());
    f.writeManifest();
    assert(runtime.available());

    assert(runtime.authorize(a,"backend",19));
    {std::ofstream replace(f.source,std::ios::trunc);
     replace<<"modified-input-material";}
    assert(runtime.execute(a)==vdrsuite::mediatools::MediaImportAgentOutcome::rejected);
    assert(fs::is_empty(f.video));
    assert(transport.checks>=3);
    verifyDurableAgentLifecycle();
}

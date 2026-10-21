#include "BackendAgentClient.h"
#include "BackendAgentCommand.h"
#include "BackendAgentCommandClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentCommandStateStore.h"
#include "MediaToolsAgentExecutor.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

using namespace vdrsuite::mediatools;

namespace {
std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

BackendAgentCommandAssignment assignment(const std::string& id,
                                         std::int64_t deadlineOffset = 60)
{
    BackendAgentCommandAssignment a;
    a.present = true;
    a.requestId = "req_" + id;
    a.correlationId = "corr_" + id;
    a.operationId = "op_" + id;
    a.jobId = "job_" + id;
    a.attemptId = "attempt_" + id;
    a.claimEpoch = 2;
    a.commandId = "command_" + id;
    a.backendId = "backend";
    a.agentId = "agent";
    a.agentInstanceId = "instance";
    a.backendGeneration = 19;
    a.commandType = kMediaImportCommandType;
    a.payloadVersion = 1;
    a.payload = "{\"sourceId\":\"inventory_17\"}";
    a.verificationPolicy = "reconciliation_required";
    a.assignedAt = nowSeconds() - 10;
    a.deadline = nowSeconds() + deadlineOffset;
    a.requestFingerprint = backendAgentCommandFingerprint(a);
    assert(backendAgentCommandValidAssignment(a));
    return a;
}

class Executor final : public IMediaImportAgentExecutor {
public:
    bool trusted = true;
    bool allowed = true;
    int dispatchCount = 0;
    int authorizationCount = 0;
    MediaImportAgentOutcome outcome = MediaImportAgentOutcome::submitted;
    bool available() override { return trusted; }
    bool authorize(const BackendAgentCommandAssignment& a,
                   const std::string& backend, std::uint64_t generation) override
    {
        ++authorizationCount;
        std::string key;
        return allowed && trusted && backend == a.backendId &&
            generation == a.backendGeneration &&
            a.verificationPolicy == "reconciliation_required" &&
            a.claimEpoch == 2 && parseMediaImportSelection(a.payload, key) &&
            key == "inventory_17";
    }
    MediaImportAgentOutcome execute(const BackendAgentCommandAssignment&) override
    {
        ++dispatchCount;
        return outcome;
    }
};

class Control final : public IBackendAgentControlPlaneTransport {
public:
    BackendAgentCommandAssignment command;
    BackendAgentCommandResult lastResult;
    int receipts = 0;
    int results = 0;
    std::vector<std::string> calls;
    BackendAgentTransportResponse postEnrollment(
        const std::string&, const std::string&, const std::string&,
        const std::string&) override { return {}; }

    BackendAgentTransportResponse postAuthenticated(
        const std::string&, const std::string&,
        const std::string& path, const std::string& body) override
    {
        calls.push_back(path);
        if (path == "/api/agent/v1/commands/poll")
        {
            BackendAgentCommandPollRequest request;
            std::string reason;
            assert(parseBackendAgentCommandPollRequestJson(body, request, reason));
            const bool advertised = std::find(request.supportedCommandTypes.begin(),
                request.supportedCommandTypes.end(), kMediaImportCommandType) !=
                request.supportedCommandTypes.end();
            if (command.present) assert(advertised);
            BackendAgentCommandPollResult response;
            response.accepted = true;
            response.reasonCode = command.present
                ? "command_assigned" : "no_command_available";
            response.assignment = command;
            return {true, 200, serializeBackendAgentCommandPollResponseJson(response), ""};
        }
        if (path == "/api/agent/v1/commands/receipt")
        {
            ++receipts;
            BackendAgentCommandReceipt r;
            std::string reason;
            assert(parseBackendAgentCommandReceiptJson(body, r, reason));
            assert(r.commandId == command.commandId);
            assert(r.claimEpoch == command.claimEpoch);
            return {true, 200, "{}", ""};
        }
        if (path == "/api/agent/v1/commands/result")
        {
            ++results;
            std::string reason;
            assert(parseBackendAgentCommandResultJson(body, lastResult, reason));
            assert(lastResult.requestFingerprint == command.requestFingerprint);
            assert(lastResult.claimEpoch == command.claimEpoch);
            return {true, 200, "{}", ""};
        }
        assert(false);
        return {};
    }
};

struct Fixture {
    std::string path;
    BackendAgentCommandClientContext context{
        "agent", "some-secret", "backend", "instance", 19};
    Executor executor;
    BackendAgentCommandClientConfig config;
    Control control;
    Fixture(const char* name) : path(std::string("/tmp/vdr-suite-a6-") + name)
    {
        std::remove(path.c_str());
        config.statePath = path;
        config.commandTypes = {kMediaImportCommandType};
        config.mediaImportExecutor = &executor;
    }
    ~Fixture() { std::remove(path.c_str()); }
};

void testStrictPayload()
{
    auto a = assignment("payload");
    std::string key;
    assert(parseMediaImportSelection(a.payload, key) && key == "inventory_17");
    for (const char* forged : {
            "{\"sourceId\":\"../../etc/passwd\"}",
            "{\"sourceId\":\"/mnt/media/a.ts\"}",
            "{\"sourceId\":\"foo bar\"}",
            "{\"sourceId\":\"inventory_17\",\"videoRoot\":\"/tmp\"}",
            "{\"sourceId\":\"\"}"})
    {
        a.payload = forged;
        a.requestFingerprint = backendAgentCommandFingerprint(a);
        assert(!backendAgentCommandValidAssignment(a));
    }
    a = assignment("policy");
    a.verificationPolicy = "none";
    a.requestFingerprint = backendAgentCommandFingerprint(a);
    assert(!backendAgentCommandValidAssignment(a));
}

void testNoImplicitActivation()
{
    Fixture f("advertisement");
    f.config.mediaImportExecutor = nullptr;
    assert(discoverBackendAgentCommandAvailability(f.config).commandTypes.empty());
    f.config.mediaImportExecutor = &f.executor;
    assert(discoverBackendAgentCommandAvailability(f.config).commandTypes ==
           std::vector<std::string>{kMediaImportCommandType});
    f.executor.trusted = false;
    assert(discoverBackendAgentCommandAvailability(f.config).commandTypes.empty());
}

void testDurableReceiptExecutionAndReplay()
{
    Fixture f("execute");
    f.control.command = assignment("normal");
    std::string reason;
    assert(pollBackendAgentCommand(f.config, f.context, f.control, reason));
    assert(f.control.receipts == 1 && f.control.results == 1);
    assert(f.executor.dispatchCount == 1);
    assert(f.control.lastResult.resultCategory == "outcome_unknown");
    assert(f.control.lastResult.verificationState == "outcome_unknown");
    assert(f.control.lastResult.retryClassification == "reconcile_only");
    assert(f.control.lastResult.dispatchState == "accepted_by_executor");
    assert(f.control.calls == std::vector<std::string>({
        "/api/agent/v1/commands/poll",
        "/api/agent/v1/commands/receipt",
        "/api/agent/v1/commands/result"}));
    assert(reconcileBackendAgentCommandState(
        f.config, f.context, f.control, reason));
    assert(f.executor.dispatchCount == 1 && f.control.results == 1);
}

void testRevokeAtExecutionBoundary()
{
    Fixture f("revoked");
    f.control.command = assignment("revoked");
    f.executor.allowed = false;
    std::string reason;
    assert(pollBackendAgentCommand(f.config, f.context, f.control, reason));
    assert(f.executor.dispatchCount == 0);
    assert(f.executor.authorizationCount == 1);
    assert(f.control.lastResult.resultCategory == "rejected");
    assert(f.control.lastResult.dispatchState == "not_started");
}

void testExpiredBeforeDispatch()
{
    Fixture f("expired");
    f.control.command = assignment("expired", -1);
    std::string reason;
    assert(pollBackendAgentCommand(f.config, f.context, f.control, reason));
    assert(f.executor.dispatchCount == 0);
    assert(f.control.lastResult.errorCategory == "expired");
}

void testCrashRecoveryNoDoubleExecution()
{
    Fixture f("crash");
    f.control.command = assignment("crash");
    vdrsuite::agent::commandstate::LocalState state;
    state.assignment = f.control.command;
    state.receipt.commandId = state.assignment.commandId;
    state.receipt.requestFingerprint = state.assignment.requestFingerprint;
    state.receipt.jobId = state.assignment.jobId;
    state.receipt.attemptId = state.assignment.attemptId;
    state.receipt.claimEpoch = state.assignment.claimEpoch;
    state.receipt.backendId = state.assignment.backendId;
    state.receipt.agentId = state.assignment.agentId;
    state.receipt.agentInstanceId = state.assignment.agentInstanceId;
    state.receipt.backendGeneration = state.assignment.backendGeneration;
    state.receipt.receiptCategory = "accepted";
    state.receipt.receivedAt = nowSeconds();
    state.receipt.reasonCode = "durably_recorded";
    state.receiptAcknowledged = true;
    state.dispatchState = "starting";
    std::string reason;
    assert(vdrsuite::agent::commandstate::persist(f.path, state, reason));
    assert(reconcileBackendAgentCommandState(
        f.config, f.context, f.control, reason));
    assert(f.executor.dispatchCount == 0);
    assert(f.control.lastResult.resultCategory == "outcome_unknown");
    assert(f.control.lastResult.retryClassification == "reconcile_only");
    assert(reconcileBackendAgentCommandState(
        f.config, f.context, f.control, reason));
    assert(f.executor.dispatchCount == 0);
    assert(f.control.results == 1);
}
}

int main()
{
    testStrictPayload();
    testNoImplicitActivation();
    testDurableReceiptExecutionAndReplay();
    testRevokeAtExecutionBoundary();
    testExpiredBeforeDispatch();
    testCrashRecoveryNoDoubleExecution();
}

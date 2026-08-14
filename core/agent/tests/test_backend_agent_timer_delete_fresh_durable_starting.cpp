#include "BackendAgentClient.h"
#include "BackendAgentCommand.h"
#include "BackendAgentCommandClient.h"
#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentNativeTimerDeleteLocalState.h"
#include "BackendAgentNativeTimerDeletePayload.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>

using namespace vdrsuite::agent;

namespace
{

class Control final : public IBackendAgentControlPlaneTransport
{
public:
    int receiptCalls = 0;
    int resultCalls = 0;
    bool failReceiptTransport = false;

    BackendAgentTransportResponse postEnrollment(
        const std::string&,
        const std::string&,
        const std::string&,
        const std::string&) override
    {
        return {};
    }

    BackendAgentTransportResponse postAuthenticated(
        const std::string&,
        const std::string&,
        const std::string& path,
        const std::string&) override
    {
        BackendAgentTransportResponse response;
        if (path.find("/receipt") != std::string::npos)
        {
            ++receiptCalls;
            if (failReceiptTransport) return response;
        }
        else if (path.find("/result") != std::string::npos)
        {
            ++resultCalls;
        }
        response.transportSucceeded = true;
        response.statusCode = 200;
        response.body = "{}";
        return response;
    }
};

BackendAgentCommandAssignment timerDeleteAssignment(std::int64_t deadline = 4102444800LL)
{
    BackendAgentLocalProviderSelection selection;
    selection.backendId = "default";
    selection.authorityDomain = kBackendAgentNativeTimerDeleteAuthorityDomain;
    selection.providerId = kBackendAgentNativeTimerDeleteProviderId;
    selection.providerKind = kBackendAgentNativeTimerDeleteProviderKind;
    selection.ownershipGeneration = 41;
    selection.providerInstanceEpoch = "suitebridge-epoch-fresh-starting";
    selection.providerGeneration = 13;
    selection.capabilityRevision = 6;
    selection.requiredCapability = kBackendAgentNativeTimerDeleteCapability;

    BackendAgentNativeTimerDeletePayload payload;
    payload.operationRevision = "operation-revision-fresh-starting";
    payload.nativeTimerBindingId = "binding-fresh-starting";
    payload.expectedBindingRevision = "binding-revision-fresh-starting";
    payload.expectedNativeTimerFingerprint = "sha256:native-timer-observed-fresh-starting";
    payload.timerAssignmentId = "timer-assignment-fresh-starting";
    payload.backendNativeTimerId = "native-timer-fresh-starting";
    payload.controlPlaneClaimedAt = 90;
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-fresh-starting";
    value.correlationId = "correlation-fresh-starting";
    value.operationId = "operation-fresh-starting";
    value.jobId = "job-fresh-starting";
    value.attemptId = "attempt-fresh-starting";
    value.claimEpoch = 5;
    value.commandId = "command-fresh-starting";
    value.backendId = "default";
    value.agentId = "agent-fresh-starting";
    value.agentInstanceId = "instance-fresh-starting";
    value.backendGeneration = 61;
    value.commandType = kBackendAgentNativeTimerDeleteCommandType;
    value.payloadVersion = kBackendAgentNativeTimerDeletePayloadVersion;
    value.payload = backendAgentNativeTimerDeletePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 100;
    value.deadline = deadline;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

std::string stateText(
    const BackendAgentCommandAssignment& assignment,
    bool receiptAcknowledged = false)
{
    std::ostringstream output;
    output << "version=3\n"
        << "protocol_version=" << assignment.protocolVersion << "\n"
        << "request_id=" << assignment.requestId << "\n"
        << "correlation_id=" << assignment.correlationId << "\n"
        << "operation_id=" << assignment.operationId << "\n"
        << "job_id=" << assignment.jobId << "\n"
        << "attempt_id=" << assignment.attemptId << "\n"
        << "claim_epoch=" << assignment.claimEpoch << "\n"
        << "command_id=" << assignment.commandId << "\n"
        << "backend_id=" << assignment.backendId << "\n"
        << "agent_id=" << assignment.agentId << "\n"
        << "agent_instance_id=" << assignment.agentInstanceId << "\n"
        << "backend_generation=" << assignment.backendGeneration << "\n"
        << "command_type=" << assignment.commandType << "\n"
        << "payload_version=" << assignment.payloadVersion << "\n"
        << "payload=" << assignment.payload << "\n"
        << "request_fingerprint=" << assignment.requestFingerprint << "\n"
        << "verification_policy=" << assignment.verificationPolicy << "\n"
        << "assigned_at=" << assignment.assignedAt << "\n"
        << "deadline=" << assignment.deadline << "\n"
        << "receipt_category=accepted\n"
        << "received_at=220\n"
        << "receipt_reason=durably_recorded\n"
        << "receipt_acknowledged=" << (receiptAcknowledged ? 1 : 0) << "\n"
        << "dispatch_state=not_started\n"
        << "result_present=0\n"
        << "result_acknowledged=0\n"
        << "verification_state=\n"
        << "result_category=\n"
        << "error_category=\n"
        << "retry_classification=\n"
        << "bounded_diagnostics=\n"
        << "completed_at=0\n"
        << "native_capability_evidence=\n"
        << "plugin_instance_epoch=\n"
        << "probe_nonce=\n"
        << "native_execution_sequence=0\n"
        << "native_receipt_evidence=\n"
        << "native_result_evidence=\n"
        << "native_readback_evidence=\n"
        << "state_extension=\n";
    return output.str();
}

void writeState(const std::string& path, const std::string& value)
{
    std::ofstream output(path);
    output << value;
    output.close();
    assert(chmod(path.c_str(), 0600) == 0);
}

std::string readAll(const std::string& path)
{
    std::ifstream input(path);
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}

std::string valueFor(const std::string& state, const std::string& key)
{
    const std::string prefix = key + "=";
    const std::size_t start = state.find(prefix);
    assert(start != std::string::npos);
    const std::size_t valueStart = start + prefix.size();
    const std::size_t end = state.find('\n', valueStart);
    return state.substr(valueStart, end - valueStart);
}

BackendAgentCommandClientContext contextFor(
    const BackendAgentCommandAssignment& assignment,
    std::uint64_t backendGeneration = 0)
{
    return BackendAgentCommandClientContext{
        assignment.agentId,
        "secret-material-at-least-thirty-two-bytes",
        assignment.backendId,
        assignment.agentInstanceId,
        backendGeneration == 0 ? assignment.backendGeneration : backendGeneration};
}

bool reconcile(
    const std::string& path,
    const BackendAgentCommandAssignment& assignment,
    Control& control,
    std::string& reason,
    std::uint64_t backendGeneration = 0)
{
    BackendAgentCommandClientConfig config{path, {"probe.noop"}};
    return reconcileBackendAgentCommandState(
        config,
        contextFor(assignment, backendGeneration),
        control,
        reason);
}

BackendAgentNativeTimerDeleteLocalState localStateFrom(
    const std::string& persisted,
    const BackendAgentCommandAssignment& assignment)
{
    BackendAgentNativeTimerDeleteLocalState state;
    std::string reason;
    assert(backendAgentNativeTimerDeleteParseCommandStateExtension(
        valueFor(persisted, "state_extension"), assignment, state, reason));
    return state;
}

}

int main()
{
    const std::string path = "/tmp/vdr-suite-timer-delete-fresh-durable-starting";
    std::remove(path.c_str());
    std::string reason;

    const auto assignment = timerDeleteAssignment();

    // Fresh current-context handoff persists typed starting before the accepted
    // receipt can be reported. No result or executor transition occurs here.
    writeState(path, stateText(assignment));
    Control control;
    assert(reconcile(path, assignment, control, reason));
    assert(reason == "native_delete_local_starting_handoff_persisted");
    assert(control.receiptCalls == 1);
    assert(control.resultCalls == 0);

    const std::string startingPersisted = readAll(path);
    assert(valueFor(startingPersisted, "dispatch_state") == "starting");
    assert(valueFor(startingPersisted, "receipt_acknowledged") == "1");
    assert(valueFor(startingPersisted, "result_present") == "0");
    assert(!valueFor(startingPersisted, "state_extension").empty());
    const auto starting = localStateFrom(startingPersisted, assignment);
    assert(starting.phase == BackendAgentNativeTimerDeleteLocalPhase::starting);
    assert(starting.localStartingPersistedAt > 0);
    assert(starting.localStartingPersistedAt <= assignment.deadline);

    struct stat status{};
    assert(lstat(path.c_str(), &status) == 0);
    assert((status.st_mode & (S_IRWXG | S_IRWXO)) == 0);

    // The next reconciliation consumes that durable hazard boundary through
    // Slice 30. It never performs a second fresh preparation or blind retry.
    assert(reconcile(path, assignment, control, reason));
    assert(reason == "command_result_reconciled");
    assert(control.receiptCalls == 1);
    assert(control.resultCalls == 1);
    const std::string recoveredPersisted = readAll(path);
    assert(valueFor(recoveredPersisted, "result_category") == "outcome_unknown");
    assert(valueFor(recoveredPersisted, "retry_classification") == "reconcile_only");
    const auto recovered = localStateFrom(recoveredPersisted, assignment);
    assert(recovered.phase == BackendAgentNativeTimerDeleteLocalPhase::completed);
    assert(recovered.evidence.outcome ==
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);
    assert(recovered.localStartingPersistedAt == starting.localStartingPersistedAt);
    std::remove(path.c_str());

    // Fresh starting is fenced by the current Agent/backend generation. A stale
    // command remains generic not_started and receives no receipt.
    writeState(path, stateText(assignment));
    Control staleControl;
    assert(!reconcile(
        path,
        assignment,
        staleControl,
        reason,
        assignment.backendGeneration + 1));
    assert(reason == "local_command_generation_fenced");
    assert(staleControl.receiptCalls == 0);
    assert(staleControl.resultCalls == 0);
    const std::string stalePersisted = readAll(path);
    assert(valueFor(stalePersisted, "dispatch_state") == "not_started");
    assert(valueFor(stalePersisted, "state_extension").empty());
    std::remove(path.c_str());

    // Expiry is checked before fresh starting. An already-expired assignment is
    // rejected without ever opening the Timer-delete hazard boundary.
    const auto expired = timerDeleteAssignment(1000);
    writeState(path, stateText(expired));
    Control expiredControl;
    assert(reconcile(path, expired, expiredControl, reason));
    assert(expiredControl.receiptCalls == 1);
    assert(expiredControl.resultCalls == 1);
    const std::string expiredPersisted = readAll(path);
    assert(valueFor(expiredPersisted, "dispatch_state") == "not_started");
    assert(valueFor(expiredPersisted, "result_category") == "rejected");
    assert(valueFor(expiredPersisted, "error_category") == "expired");
    assert(valueFor(expiredPersisted, "state_extension").empty());
    std::remove(path.c_str());

    // Receipt transport failure after the durable starting write cannot reopen
    // fresh preparation. Recovery converts the same starting evidence to
    // outcome_unknown/reconcile_only and then replays receipt/result safely.
    writeState(path, stateText(assignment));
    Control failedReceipt;
    failedReceipt.failReceiptTransport = true;
    assert(!reconcile(path, assignment, failedReceipt, reason));
    assert(reason == "command_transport_failed");
    assert(failedReceipt.receiptCalls == 1);
    assert(failedReceipt.resultCalls == 0);
    const std::string afterReceiptFailure = readAll(path);
    assert(valueFor(afterReceiptFailure, "dispatch_state") == "starting");
    assert(valueFor(afterReceiptFailure, "receipt_acknowledged") == "0");
    const auto failedStarting = localStateFrom(afterReceiptFailure, assignment);
    assert(failedStarting.phase == BackendAgentNativeTimerDeleteLocalPhase::starting);

    Control recoveryControl;
    assert(reconcile(path, assignment, recoveryControl, reason));
    assert(reason == "command_result_reconciled");
    assert(recoveryControl.receiptCalls == 1);
    assert(recoveryControl.resultCalls == 1);
    const std::string afterRecovery = readAll(path);
    const auto completed = localStateFrom(afterRecovery, assignment);
    assert(completed.phase == BackendAgentNativeTimerDeleteLocalPhase::completed);
    assert(completed.evidence.outcome ==
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);
    assert(completed.localStartingPersistedAt ==
        failedStarting.localStartingPersistedAt);
    assert(valueFor(afterRecovery, "retry_classification") == "reconcile_only");
    std::remove(path.c_str());

    return 0;
}

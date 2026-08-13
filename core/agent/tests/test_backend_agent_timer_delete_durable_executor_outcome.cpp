#include "BackendAgentClient.h"
#include "BackendAgentCommand.h"
#include "BackendAgentCommandClient.h"
#include "BackendAgentNativeTimerDeleteExecutor.h"
#include "BackendAgentNativeTimerDeleteLocalState.h"
#include "BackendAgentNativeTimerDeletePayload.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <stdexcept>
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
    bool failResultTransport = false;

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
            if (failResultTransport) return response;
        }
        response.transportSucceeded = true;
        response.statusCode = 200;
        response.body = "{}";
        return response;
    }
};

BackendAgentCommandAssignment timerDeleteAssignment()
{
    BackendAgentLocalProviderSelection selection;
    selection.backendId = "default";
    selection.authorityDomain = kBackendAgentNativeTimerDeleteAuthorityDomain;
    selection.providerId = kBackendAgentNativeTimerDeleteProviderId;
    selection.providerKind = kBackendAgentNativeTimerDeleteProviderKind;
    selection.ownershipGeneration = 41;
    selection.providerInstanceEpoch = "suitebridge-epoch-durable-outcome";
    selection.providerGeneration = 13;
    selection.capabilityRevision = 6;
    selection.requiredCapability = kBackendAgentNativeTimerDeleteCapability;

    BackendAgentNativeTimerDeletePayload payload;
    payload.operationRevision = "operation-revision-durable-outcome";
    payload.nativeTimerBindingId = "binding-durable-outcome";
    payload.expectedBindingRevision = "binding-revision-durable-outcome";
    payload.timerAssignmentId = "timer-assignment-durable-outcome";
    payload.backendNativeTimerId = "native-timer-durable-outcome";
    payload.controlPlaneClaimedAt = 90;
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-durable-outcome";
    value.correlationId = "correlation-durable-outcome";
    value.operationId = "operation-durable-outcome";
    value.jobId = "job-durable-outcome";
    value.attemptId = "attempt-durable-outcome";
    value.claimEpoch = 5;
    value.commandId = "command-durable-outcome";
    value.backendId = "default";
    value.agentId = "agent-durable-outcome";
    value.agentInstanceId = "instance-durable-outcome";
    value.backendGeneration = 61;
    value.commandType = kBackendAgentNativeTimerDeleteCommandType;
    value.payloadVersion = kBackendAgentNativeTimerDeletePayloadVersion;
    value.payload = backendAgentNativeTimerDeletePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 100;
    value.deadline = 4102444800LL;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentLocalProviderFacts factsFor(
    const BackendAgentCommandAssignment& assignment)
{
    BackendAgentNativeTimerDeleteCommand command;
    std::string reason;
    assert(backendAgentNativeTimerDeleteCommandFromAssignment(
        assignment, command, reason));
    BackendAgentLocalProviderFacts facts;
    facts.providerId = command.localProviderSelection.providerId;
    facts.providerKind = command.localProviderSelection.providerKind;
    facts.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
    facts.providerGeneration = command.localProviderSelection.providerGeneration;
    facts.capabilityRevision = command.localProviderSelection.capabilityRevision;
    facts.available = true;
    facts.capabilities = {command.localProviderSelection.requiredCapability};
    assert(backendAgentLocalProviderValidFacts(facts));
    return facts;
}

class DeleteTransport final : public IBackendAgentNativeTimerDeleteTransport
{
public:
    BackendAgentLocalProviderFacts facts;
    BackendAgentNativeTimerDeleteTransportDisposition disposition =
        BackendAgentNativeTimerDeleteTransportDisposition::acceptedUnverified;
    std::string evidenceReference = "fake-delete:accepted";
    bool discoverySucceeds = true;
    bool throwDelete = false;
    int discoveryCalls = 0;
    int deleteCalls = 0;
    BackendAgentNativeTimerDeleteTransportRequest lastRequest;

    bool discoverProvider(
        BackendAgentLocalProviderFacts& value,
        std::string& reasonCode) override
    {
        ++discoveryCalls;
        if (!discoverySucceeds)
        {
            reasonCode = "fake-provider-discovery-failed";
            return false;
        }
        value = facts;
        reasonCode = "fake-provider-current";
        return true;
    }

    BackendAgentNativeTimerDeleteTransportReply deleteTimer(
        const BackendAgentNativeTimerDeleteTransportRequest& request) override
    {
        ++deleteCalls;
        lastRequest = request;
        if (throwDelete) throw std::runtime_error("fake ambiguous dispatch");
        BackendAgentNativeTimerDeleteTransportReply reply;
        reply.disposition = disposition;
        reply.evidenceReference = evidenceReference;
        return reply;
    }
};

std::string stateText(const BackendAgentCommandAssignment& assignment)
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
        << "receipt_acknowledged=0\n"
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

BackendAgentCommandClientContext contextFor(
    const BackendAgentCommandAssignment& assignment)
{
    return BackendAgentCommandClientContext{
        assignment.agentId,
        "secret-material-at-least-thirty-two-bytes",
        assignment.backendId,
        assignment.agentInstanceId,
        assignment.backendGeneration};
}

bool reconcile(
    const std::string& path,
    const BackendAgentCommandAssignment& assignment,
    Control& control,
    DeleteTransport* deleteTransport,
    std::string& reason)
{
    BackendAgentCommandClientConfig config;
    config.statePath = path;
    config.commandTypes = {"probe.noop"};
    config.nativeTimerDeleteTransport = deleteTransport;
    return reconcileBackendAgentCommandState(
        config, contextFor(assignment), control, reason);
}

}

int main()
{
    const std::string path =
        "/tmp/vdr-suite-timer-delete-durable-executor-outcome";
    std::remove(path.c_str());
    std::string reason;
    const auto assignment = timerDeleteAssignment();

    // With no injected Timer-delete transport, the shipped/default path keeps
    // the exact Slice-31 boundary: durable starting + receipt, no executor.
    writeState(path, stateText(assignment));
    Control disabledControl;
    assert(reconcile(path, assignment, disabledControl, nullptr, reason));
    assert(reason == "native_delete_local_starting_handoff_persisted");
    assert(disabledControl.receiptCalls == 1);
    assert(disabledControl.resultCalls == 0);
    const auto disabledState = localStateFrom(readAll(path), assignment);
    assert(disabledState.phase == BackendAgentNativeTimerDeleteLocalPhase::starting);
    std::remove(path.c_str());

    // Fresh durable starting -> accepted receipt -> exactly one executor call ->
    // durable typed completed evidence + generic result -> result delivery.
    writeState(path, stateText(assignment));
    Control acceptedControl;
    DeleteTransport accepted;
    accepted.facts = factsFor(assignment);
    assert(reconcile(path, assignment, acceptedControl, &accepted, reason));
    assert(reason == "native_delete_executor_outcome_reconciled");
    assert(acceptedControl.receiptCalls == 1);
    assert(acceptedControl.resultCalls == 1);
    assert(accepted.discoveryCalls == 1);
    assert(accepted.deleteCalls == 1);
    const std::string acceptedPersisted = readAll(path);
    const auto acceptedState = localStateFrom(acceptedPersisted, assignment);
    assert(acceptedState.phase == BackendAgentNativeTimerDeleteLocalPhase::completed);
    assert(acceptedState.evidence.outcome ==
        BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified);
    assert(acceptedState.evidence.dispatchStartedAt >=
        acceptedState.localStartingPersistedAt);
    assert(valueFor(acceptedPersisted, "dispatch_state") ==
        "accepted_by_executor");
    assert(valueFor(acceptedPersisted, "result_category") == "outcome_unknown");
    assert(valueFor(acceptedPersisted, "retry_classification") ==
        "reconcile_only");
    assert(valueFor(acceptedPersisted, "result_acknowledged") == "1");

    // Completed evidence is replay authority. A later reconcile never invokes
    // the executor again, even while an injected transport is still present.
    Control acceptedReplayControl;
    assert(reconcile(
        path, assignment, acceptedReplayControl, &accepted, reason));
    assert(accepted.deleteCalls == 1);
    assert(acceptedReplayControl.receiptCalls == 0);
    assert(acceptedReplayControl.resultCalls == 0);
    std::remove(path.c_str());

    // Provider drift is decided before dispatch, then the rejected-without-
    // effect evidence and its generic projection are durably conserved.
    writeState(path, stateText(assignment));
    Control fencedControl;
    DeleteTransport fenced;
    fenced.facts = factsFor(assignment);
    ++fenced.facts.providerGeneration;
    assert(reconcile(path, assignment, fencedControl, &fenced, reason));
    assert(fenced.discoveryCalls == 1);
    assert(fenced.deleteCalls == 0);
    const std::string fencedPersisted = readAll(path);
    const auto fencedState = localStateFrom(fencedPersisted, assignment);
    assert(fencedState.phase == BackendAgentNativeTimerDeleteLocalPhase::completed);
    assert(fencedState.evidence.outcome ==
        BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect);
    assert(valueFor(fencedPersisted, "dispatch_state") == "not_started");
    assert(valueFor(fencedPersisted, "result_category") == "rejected");
    assert(valueFor(fencedPersisted, "error_category") == "fenced");
    std::remove(path.c_str());

    // An ambiguous post-dispatch exception becomes durable outcome_unknown.
    // Reconciliation may replay evidence/results, but never the delete call.
    writeState(path, stateText(assignment));
    Control unknownControl;
    DeleteTransport unknown;
    unknown.facts = factsFor(assignment);
    unknown.throwDelete = true;
    assert(reconcile(path, assignment, unknownControl, &unknown, reason));
    assert(unknown.deleteCalls == 1);
    const std::string unknownPersisted = readAll(path);
    const auto unknownState = localStateFrom(unknownPersisted, assignment);
    assert(unknownState.evidence.outcome ==
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);
    assert(valueFor(unknownPersisted, "dispatch_state") == "starting");
    assert(valueFor(unknownPersisted, "error_category") == "executor_unknown");
    Control unknownReplayControl;
    assert(reconcile(path, assignment, unknownReplayControl, &unknown, reason));
    assert(unknown.deleteCalls == 1);
    std::remove(path.c_str());

    // Result transport can fail only after completed evidence and the generic
    // result are durable. A later result replay never re-enters the executor.
    writeState(path, stateText(assignment));
    Control resultFailureControl;
    resultFailureControl.failResultTransport = true;
    DeleteTransport resultFailure;
    resultFailure.facts = factsFor(assignment);
    assert(!reconcile(
        path, assignment, resultFailureControl, &resultFailure, reason));
    assert(reason == "command_transport_failed");
    assert(resultFailure.deleteCalls == 1);
    const std::string afterResultFailure = readAll(path);
    const auto resultFailureState =
        localStateFrom(afterResultFailure, assignment);
    assert(resultFailureState.phase ==
        BackendAgentNativeTimerDeleteLocalPhase::completed);
    assert(resultFailureState.evidence.outcome ==
        BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified);
    assert(valueFor(afterResultFailure, "result_present") == "1");
    assert(valueFor(afterResultFailure, "result_acknowledged") == "0");
    Control resultReplayControl;
    assert(reconcile(
        path, assignment, resultReplayControl, &resultFailure, reason));
    assert(resultFailure.deleteCalls == 1);
    assert(resultReplayControl.resultCalls == 1);
    assert(valueFor(readAll(path), "result_acknowledged") == "1");
    std::remove(path.c_str());

    // Receipt transport loss occurs before executor dispatch. The existing
    // durable starting state is recovered as outcome_unknown on the next pass;
    // even with a transport available then, there is no blind delete retry.
    writeState(path, stateText(assignment));
    Control receiptFailureControl;
    receiptFailureControl.failReceiptTransport = true;
    DeleteTransport receiptFailure;
    receiptFailure.facts = factsFor(assignment);
    assert(!reconcile(
        path, assignment, receiptFailureControl, &receiptFailure, reason));
    assert(reason == "command_transport_failed");
    assert(receiptFailure.deleteCalls == 0);
    const std::string afterReceiptFailure = readAll(path);
    const auto receiptStarting =
        localStateFrom(afterReceiptFailure, assignment);
    assert(receiptStarting.phase ==
        BackendAgentNativeTimerDeleteLocalPhase::starting);
    assert(valueFor(afterReceiptFailure, "receipt_acknowledged") == "0");
    Control receiptRecoveryControl;
    assert(reconcile(
        path, assignment, receiptRecoveryControl, &receiptFailure, reason));
    assert(receiptFailure.deleteCalls == 0);
    const auto receiptRecovered = localStateFrom(readAll(path), assignment);
    assert(receiptRecovered.phase ==
        BackendAgentNativeTimerDeleteLocalPhase::completed);
    assert(receiptRecovered.evidence.outcome ==
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);
    assert(receiptRecoveryControl.receiptCalls == 1);
    assert(receiptRecoveryControl.resultCalls == 1);
    std::remove(path.c_str());

    return 0;
}

#include "BackendAgentClient.h"
#include "BackendAgentCommand.h"
#include "BackendAgentCommandClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentNativeTimerDeletePayload.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace vdrsuite::agent;

namespace
{

class Control final : public IBackendAgentControlPlaneTransport
{
public:
    int receiptCalls = 0;
    int resultCalls = 0;
    int pollCalls = 0;
    bool assertTimerDeleteSuppressed = false;

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
        const std::string& body) override
    {
        BackendAgentTransportResponse response;
        response.transportSucceeded = true;
        response.statusCode = 200;
        if (path.find("/poll") != std::string::npos)
        {
            ++pollCalls;
            BackendAgentCommandPollRequest request;
            std::string reason;
            assert(parseBackendAgentCommandPollRequestJson(
                body, request, reason));
            if (assertTimerDeleteSuppressed)
            {
                assert(request.supportedCommandTypes ==
                    std::vector<std::string>{"probe.noop"});
            }
            BackendAgentCommandPollResult result;
            result.accepted = true;
            result.reasonCode = "no_command_available";
            response.body =
                serializeBackendAgentCommandPollResponseJson(result);
        }
        else if (path.find("/receipt") != std::string::npos)
        {
            ++receiptCalls;
            response.body = "{}";
        }
        else if (path.find("/result") != std::string::npos)
        {
            ++resultCalls;
            response.body = "{}";
        }
        return response;
    }
};

BackendAgentCommandAssignment probeAssignment()
{
    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-v3-probe";
    value.correlationId = "correlation-v3-probe";
    value.operationId = "operation-v3-probe";
    value.jobId = "job-v3-probe";
    value.attemptId = "attempt-v3-probe";
    value.claimEpoch = 2;
    value.commandId = "command-v3-probe";
    value.backendId = "default";
    value.agentId = "agent-v3";
    value.agentInstanceId = "instance-v3";
    value.backendGeneration = 51;
    value.commandType = "probe.noop";
    value.payloadVersion = 1;
    value.payload = "{}";
    value.verificationPolicy = "none";
    value.assignedAt = 100;
    value.deadline = 4102444800LL;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentCommandAssignment timerDeleteAssignment()
{
    BackendAgentLocalProviderSelection selection;
    selection.backendId = "default";
    selection.authorityDomain = kBackendAgentNativeTimerDeleteAuthorityDomain;
    selection.providerId = kBackendAgentNativeTimerDeleteProviderId;
    selection.providerKind = kBackendAgentNativeTimerDeleteProviderKind;
    selection.ownershipGeneration = 31;
    selection.providerInstanceEpoch = "suitebridge-epoch-v3";
    selection.providerGeneration = 12;
    selection.capabilityRevision = 5;
    selection.requiredCapability = kBackendAgentNativeTimerDeleteCapability;

    BackendAgentNativeTimerDeletePayload payload;
    payload.operationRevision = "operation-revision-v3";
    payload.nativeTimerBindingId = "binding-v3";
    payload.expectedBindingRevision = "binding-revision-v3";
    payload.timerAssignmentId = "timer-assignment-v3";
    payload.backendNativeTimerId = "native-timer-v3";
    payload.controlPlaneClaimedAt = 200;
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-v3-delete";
    value.correlationId = "correlation-v3-delete";
    value.operationId = "operation-v3-delete";
    value.jobId = "job-v3-delete";
    value.attemptId = "attempt-v3-delete";
    value.claimEpoch = 4;
    value.commandId = "command-v3-delete";
    value.backendId = "default";
    value.agentId = "agent-v3";
    value.agentInstanceId = "instance-v3";
    value.backendGeneration = 51;
    value.commandType = kBackendAgentNativeTimerDeleteCommandType;
    value.payloadVersion = kBackendAgentNativeTimerDeletePayloadVersion;
    value.payload = backendAgentNativeTimerDeletePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 210;
    value.deadline = 4102444800LL;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

std::string stateText(
    int version,
    const BackendAgentCommandAssignment& assignment,
    const std::string& extension,
    bool completed = true,
    bool receiptAcknowledged = true,
    bool resultAcknowledged = true)
{
    assert(version >= 1 && version <= 3);
    std::ostringstream output;
    output << "version=" << version
        << "\nprotocol_version=" << assignment.protocolVersion
        << "\nrequest_id=" << assignment.requestId
        << "\ncorrelation_id=" << assignment.correlationId
        << "\noperation_id=" << assignment.operationId
        << "\njob_id=" << assignment.jobId
        << "\nattempt_id=" << assignment.attemptId
        << "\nclaim_epoch=" << assignment.claimEpoch
        << "\ncommand_id=" << assignment.commandId
        << "\nbackend_id=" << assignment.backendId
        << "\nagent_id=" << assignment.agentId
        << "\nagent_instance_id=" << assignment.agentInstanceId
        << "\nbackend_generation=" << assignment.backendGeneration
        << "\ncommand_type=" << assignment.commandType
        << "\npayload_version=" << assignment.payloadVersion
        << "\npayload=" << assignment.payload
        << "\nrequest_fingerprint=" << assignment.requestFingerprint
        << "\nverification_policy=" << assignment.verificationPolicy
        << "\nassigned_at=" << assignment.assignedAt
        << "\ndeadline=" << assignment.deadline
        << "\nreceipt_category=accepted"
        << "\nreceived_at=220"
        << "\nreceipt_reason=durably_recorded"
        << "\nreceipt_acknowledged=" << (receiptAcknowledged ? 1 : 0)
        << "\ndispatch_state=" << (completed ? "effect_reported" : "not_started")
        << "\nresult_present=" << (completed ? 1 : 0)
        << "\nresult_acknowledged=" << (resultAcknowledged ? 1 : 0)
        << "\nverification_state=" << (completed ? "not_required" : "")
        << "\nresult_category=" << (completed ? "succeeded" : "")
        << "\nerror_category=" << (completed ? "none" : "")
        << "\nretry_classification=" << (completed ? "none" : "")
        << "\nbounded_diagnostics="
        << (completed ? "probe.noop completed without native side effect" : "")
        << "\ncompleted_at=" << (completed ? 230 : 0);

    if (version >= 2)
    {
        output << "\nnative_capability_evidence="
            << "\nplugin_instance_epoch="
            << "\nprobe_nonce="
            << "\nnative_execution_sequence=0"
            << "\nnative_receipt_evidence="
            << "\nnative_result_evidence="
            << "\nnative_readback_evidence=";
    }
    if (version == 3)
        output << "\nstate_extension=" << extension;
    output << "\n";
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

std::string hexEncode(const std::string& value)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(value.size() * 2U);
    for (unsigned char character : value)
    {
        encoded.push_back(digits[(character >> 4U) & 0x0fU]);
        encoded.push_back(digits[character & 0x0fU]);
    }
    return encoded;
}

std::vector<std::string> split(const std::string& value, char separator)
{
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (start <= value.size())
    {
        const std::size_t next = value.find(separator, start);
        fields.push_back(value.substr(
            start,
            next == std::string::npos ? std::string::npos : next - start));
        if (next == std::string::npos) break;
        start = next + 1U;
    }
    return fields;
}

std::string join(const std::vector<std::string>& fields, char separator)
{
    std::ostringstream output;
    for (std::size_t index = 0; index < fields.size(); ++index)
    {
        if (index != 0) output << separator;
        output << fields[index];
    }
    return output.str();
}

bool reconcile(
    const std::string& path,
    const BackendAgentCommandAssignment& assignment,
    Control& control,
    std::string& reason)
{
    BackendAgentCommandClientConfig config{path, {"probe.noop"}};
    BackendAgentCommandClientContext context{
        assignment.agentId,
        "secret-material-at-least-thirty-two-bytes",
        assignment.backendId,
        assignment.agentInstanceId,
        assignment.backendGeneration};
    return reconcileBackendAgentCommandState(
        config, context, control, reason);
}

void expectAcceptedLoad(
    const std::string& path,
    int version,
    const BackendAgentCommandAssignment& assignment)
{
    std::remove(path.c_str());
    writeState(path, stateText(version, assignment, ""));
    Control control;
    std::string reason;
    assert(reconcile(path, assignment, control, reason));
    assert(reason == "command_result_reconciled");
    assert(control.receiptCalls == 0);
    assert(control.resultCalls == 0);
    std::remove(path.c_str());
}

void expectRejectedState(
    const std::string& path,
    const std::string& encodedState,
    const BackendAgentCommandAssignment& assignment,
    const std::string& expectedReason)
{
    std::remove(path.c_str());
    writeState(path, encodedState);
    Control control;
    std::string reason;
    assert(!reconcile(path, assignment, control, reason));
    assert(reason == expectedReason);
    assert(control.receiptCalls == 0);
    assert(control.resultCalls == 0);
    std::remove(path.c_str());
}

}

int main()
{
    const std::string path = "/tmp/vdr-suite-command-state-v3";
    std::remove(path.c_str());
    const auto probe = probeAssignment();
    const auto timerDelete = timerDeleteAssignment();
    std::string reason;

    // Exact v1 and v2 formats remain readable; v3 without an extension is valid.
    expectAcceptedLoad(path, 1, probe);
    expectAcceptedLoad(path, 2, probe);
    expectAcceptedLoad(path, 3, probe);

    BackendAgentNativeTimerDeleteLocalState starting;
    assert(backendAgentNativeTimerDeletePrepareLocalStarting(
        timerDelete, 220, starting, reason));
    const std::string extension =
        backendAgentNativeTimerDeleteCommandStateExtension(
            timerDelete, starting, reason);
    assert(!extension.empty());

    // A valid typed extension is loaded, survives a normal state-owner persist,
    // and still cannot cause Timer-delete execution in this slice.
    writeState(
        path,
        stateText(3, timerDelete, extension, false, false, false));
    Control timerControl;
    assert(!reconcile(path, timerDelete, timerControl, reason));
    assert(reason == "unsupported_local_command_state");
    assert(timerControl.receiptCalls == 1);
    assert(timerControl.resultCalls == 0);
    const std::string persisted = readAll(path);
    assert(persisted.rfind("version=3\n", 0) == 0);
    assert(valueFor(persisted, "state_extension") == extension);
    assert(persisted.find("\ntimer_delete_") == std::string::npos);
    struct stat status{};
    assert(lstat(path.c_str(), &status) == 0);
    assert((status.st_mode & (S_IRWXG | S_IRWXO)) == 0);

    BackendAgentNativeTimerDeleteLocalState recovered;
    assert(backendAgentNativeTimerDeleteParseCommandStateExtension(
        valueFor(persisted, "state_extension"),
        timerDelete,
        recovered,
        reason));
    assert(recovered.phase ==
        BackendAgentNativeTimerDeleteLocalPhase::starting);
    assert(recovered.command.commandId == timerDelete.commandId);
    assert(recovered.command.requestFingerprint ==
        timerDelete.requestFingerprint);
    std::remove(path.c_str());

    // Cross-command adoption is fail-closed.
    auto different = timerDelete;
    different.commandId = "command-v3-delete-other";
    different.requestId = "request-v3-delete-other";
    different.requestFingerprint = backendAgentCommandFingerprint(different);
    assert(backendAgentCommandValidAssignment(different));
    expectRejectedState(
        path,
        stateText(3, different, extension, false, true, false),
        different,
        "command_state_invalid_extension");

    // A mismatching fingerprint inside cse1 is rejected even when the enclosing
    // assignment itself remains valid.
    auto fields = split(extension, '.');
    assert(fields.size() == 5);
    fields[3] = hexEncode("fp1_deadbeefdeadbeef");
    expectRejectedState(
        path,
        stateText(3, timerDelete, join(fields, '.'), false, true, false),
        timerDelete,
        "command_state_invalid_extension");

    // Syntactically valid but unknown mutation-bearing extension types do not
    // gain forward-compatible authority accidentally.
    BackendAgentCommandStateExtension unknown;
    unknown.extensionType = "future.mutation.local-state.v1";
    unknown.commandId = timerDelete.commandId;
    unknown.requestFingerprint = timerDelete.requestFingerprint;
    unknown.payload = "opaque";
    const std::string unknownEncoded =
        backendAgentCommandStateExtensionSerialize(
            unknown, timerDelete, reason);
    assert(!unknownEncoded.empty());
    expectRejectedState(
        path,
        stateText(3, timerDelete, unknownEncoded, false, true, false),
        timerDelete,
        "command_state_invalid_extension");

    expectRejectedState(
        path,
        stateText(3, timerDelete, "cse1.malformed", false, true, false),
        timerDelete,
        "command_state_invalid_extension");

    expectRejectedState(
        path,
        stateText(
            3,
            timerDelete,
            std::string("cse1.") + std::string(40U * 1024U, 'a'),
            false,
            true,
            false),
        timerDelete,
        "command_state_invalid_extension");

    // Newline/key injection cannot escape the exact-key parser.
    expectRejectedState(
        path,
        stateText(
            3,
            timerDelete,
            extension + "\ninjected_key=1",
            false,
            true,
            false),
        timerDelete,
        "command_state_invalid");

    // Duplicate keys remain rejected before any lifecycle action.
    expectRejectedState(
        path,
        stateText(3, probe, "") + "command_id=duplicate\n",
        probe,
        "command_state_invalid");

    // v3 is exact-key parsed as well; no unknown top-level fields are tolerated.
    expectRejectedState(
        path,
        stateText(3, probe, "") + "future_state_field=1\n",
        probe,
        "command_state_invalid");

    // Even a programmatically injected commandTypes list cannot advertise
    // Timer-delete before the separately gated execution slice.
    BackendAgentCommandClientConfig pollConfig{
        path,
        {"probe.noop", kBackendAgentNativeTimerDeleteCommandType}};
    BackendAgentCommandClientContext pollContext{
        probe.agentId,
        "secret-material-at-least-thirty-two-bytes",
        probe.backendId,
        probe.agentInstanceId,
        probe.backendGeneration};
    Control pollControl;
    pollControl.assertTimerDeleteSuppressed = true;
    assert(pollBackendAgentCommand(
        pollConfig, pollContext, pollControl, reason));
    assert(reason == "no_command_available");
    assert(pollControl.pollCalls == 1);
    std::remove(path.c_str());

    return 0;
}

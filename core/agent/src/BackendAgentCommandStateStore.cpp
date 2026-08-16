#include "BackendAgentCommandStateStore.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace vdrsuite::agent::commandstate
{
namespace
{

constexpr std::size_t MaximumStateBytes = 64U * 1024U;

bool syncParent(const std::string& path)
{
    const auto separator = path.find_last_of('/');
    const std::string parent = separator == std::string::npos
        ? "." : separator == 0 ? "/" : path.substr(0, separator);
    const int descriptor = open(
        parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0) return false;
    const bool synced = fsync(descriptor) == 0;
    const bool closed = close(descriptor) == 0;
    return synced && closed;
}

bool writeAll(int descriptor, const std::string& value)
{
    std::size_t offset = 0;
    while (offset < value.size())
    {
        const ssize_t written = write(
            descriptor, value.data() + offset, value.size() - offset);
        if (written < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (written == 0) return false;
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool writeProtected(
    const std::string& path,
    const std::string& value,
    std::string& reason)
{
    if (path.empty() || path.front() != '/' ||
        value.empty() || value.size() > MaximumStateBytes)
    {
        reason = "invalid_command_state_path";
        return false;
    }
    const std::string temporary = path + ".tmp";
    const int descriptor = open(
        temporary.c_str(),
        O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW,
        0600);
    if (descriptor < 0)
    {
        reason = "command_state_open_failed";
        return false;
    }
    const bool complete = fchmod(descriptor, 0600) == 0 &&
        writeAll(descriptor, value) && fsync(descriptor) == 0;
    const bool closed = close(descriptor) == 0;
    if (!complete || !closed || rename(temporary.c_str(), path.c_str()) != 0 ||
        !syncParent(path))
    {
        unlink(temporary.c_str());
        reason = "command_state_persist_failed";
        return false;
    }
    reason = "command_state_persisted";
    return true;
}

bool readStateFile(
    const std::string& path,
    std::map<std::string, std::string>& values,
    std::string& reason)
{
    values.clear();
    struct stat status{};
    if (lstat(path.c_str(), &status) != 0)
    {
        reason = errno == ENOENT
            ? "command_state_not_found" : "command_state_stat_failed";
        return false;
    }
    if (!S_ISREG(status.st_mode) ||
        (status.st_mode & (S_IRWXG | S_IRWXO)) != 0 ||
        status.st_size < 0 ||
        static_cast<std::size_t>(status.st_size) > MaximumStateBytes)
    {
        reason = "command_state_unprotected";
        return false;
    }
    std::ifstream input(path);
    if (!input)
    {
        reason = "command_state_open_failed";
        return false;
    }
    std::string line;
    while (std::getline(input, line))
    {
        const auto separator = line.find('=');
        if (separator == std::string::npos || separator == 0 ||
            !values.emplace(
                line.substr(0, separator),
                line.substr(separator + 1)).second)
        {
            reason = "command_state_invalid";
            return false;
        }
    }
    return true;
}

bool number(const std::string& value, std::uint64_t& parsed)
{
    if (value.empty() || value.size() > 19) return false;
    parsed = 0;
    for (unsigned char character : value)
    {
        if (character < '0' || character > '9') return false;
        const unsigned digit = static_cast<unsigned>(character - '0');
        if (parsed >
            (static_cast<std::uint64_t>(INT64_MAX) - digit) / 10U) return false;
        parsed = parsed * 10U + digit;
    }
    return true;
}

const std::vector<std::string>& legacyKeys()
{
    static const std::vector<std::string> keys = {
        "version", "protocol_version", "request_id", "correlation_id",
        "operation_id", "job_id", "attempt_id", "claim_epoch", "command_id",
        "backend_id", "agent_id", "agent_instance_id", "backend_generation",
        "command_type", "payload_version", "payload", "request_fingerprint",
        "verification_policy", "assigned_at", "deadline", "receipt_category",
        "received_at", "receipt_reason", "receipt_acknowledged", "dispatch_state",
        "result_present", "result_acknowledged", "verification_state",
        "result_category", "error_category", "retry_classification",
        "bounded_diagnostics", "completed_at"};
    return keys;
}

const std::vector<std::string>& currentKeys()
{
    static const std::vector<std::string> keys = [] {
        std::vector<std::string> value = legacyKeys();
        value.insert(value.end(), {
            "native_capability_evidence", "plugin_instance_epoch", "probe_nonce",
            "native_execution_sequence", "native_receipt_evidence",
            "native_result_evidence", "native_readback_evidence"});
        return value;
    }();
    return keys;
}

const std::vector<std::string>& extendedKeys()
{
    static const std::vector<std::string> keys = [] {
        std::vector<std::string> value = currentKeys();
        value.push_back("state_extension");
        return value;
    }();
    return keys;
}

bool exactKeys(
    const std::map<std::string, std::string>& values,
    const std::vector<std::string>& keys)
{
    if (values.size() != keys.size()) return false;
    return std::all_of(keys.begin(), keys.end(), [&](const std::string& key) {
        return values.count(key) == 1;
    });
}

std::string boolText(bool value)
{
    return value ? "1" : "0";
}

}

bool retireProtectedState(const std::string& path, std::string& reason)
{
    if (path.empty() || path.front() != '/')
    {
        reason = "invalid_command_state_path";
        return false;
    }
    struct stat status{};
    if (lstat(path.c_str(), &status) != 0)
    {
        reason = errno == ENOENT
            ? "command_state_not_found" : "command_state_stat_failed";
        return errno == ENOENT;
    }
    if (!S_ISREG(status.st_mode) ||
        (status.st_mode & (S_IRWXG | S_IRWXO)) != 0)
    {
        reason = "command_state_unprotected";
        return false;
    }
    if (unlink(path.c_str()) != 0 || !syncParent(path))
    {
        reason = "command_state_retire_failed";
        return false;
    }
    reason = "completed_command_state_retired";
    return true;
}

bool load(const std::string& path, LocalState& state, std::string& reason)
{
    std::map<std::string, std::string> values;
    if (!readStateFile(path, values, reason)) return false;
    const bool legacy = values["version"] == "1";
    const bool current = values["version"] == "2";
    const bool extended = values["version"] == "3";
    const bool nativeState = current || extended;
    const auto& expectedKeys = legacy
        ? legacyKeys() : current ? currentKeys() : extendedKeys();
    if ((!legacy && !current && !extended) ||
        !exactKeys(values, expectedKeys))
    {
        reason = "command_state_invalid";
        return false;
    }

    std::uint64_t claim=0, generation=0, payloadVersion=0,
        assigned=0, deadline=0, received=0, completed=0, nativeSequence=0;
    if (!number(values["claim_epoch"], claim) ||
        !number(values["backend_generation"], generation) ||
        !number(values["payload_version"], payloadVersion) ||
        !number(values["assigned_at"], assigned) ||
        !number(values["deadline"], deadline) ||
        !number(values["received_at"], received) ||
        !number(values["completed_at"], completed) ||
        (nativeState &&
         !number(values["native_execution_sequence"], nativeSequence)))
    {
        reason = "command_state_invalid";
        return false;
    }

    auto& assignment = state.assignment;
    assignment.present = true;
    assignment.protocolVersion = values["protocol_version"];
    assignment.requestId = values["request_id"];
    assignment.correlationId = values["correlation_id"];
    assignment.operationId = values["operation_id"];
    assignment.jobId = values["job_id"];
    assignment.attemptId = values["attempt_id"];
    assignment.claimEpoch = claim;
    assignment.commandId = values["command_id"];
    assignment.backendId = values["backend_id"];
    assignment.agentId = values["agent_id"];
    assignment.agentInstanceId = values["agent_instance_id"];
    assignment.backendGeneration = generation;
    assignment.commandType = values["command_type"];
    assignment.payloadVersion = payloadVersion;
    assignment.payload = values["payload"];
    assignment.requestFingerprint = values["request_fingerprint"];
    assignment.verificationPolicy = values["verification_policy"];
    assignment.assignedAt = static_cast<std::int64_t>(assigned);
    assignment.deadline = static_cast<std::int64_t>(deadline);
    if (!backendAgentCommandValidAssignment(assignment))
    {
        reason = "command_state_invalid_assignment";
        return false;
    }

    auto& receipt = state.receipt;
    receipt.commandId = assignment.commandId;
    receipt.requestFingerprint = assignment.requestFingerprint;
    receipt.jobId = assignment.jobId;
    receipt.attemptId = assignment.attemptId;
    receipt.claimEpoch = assignment.claimEpoch;
    receipt.backendId = assignment.backendId;
    receipt.agentId = assignment.agentId;
    receipt.agentInstanceId = assignment.agentInstanceId;
    receipt.backendGeneration = assignment.backendGeneration;
    receipt.receiptCategory = values["receipt_category"];
    receipt.receivedAt = static_cast<std::int64_t>(received);
    receipt.reasonCode = values["receipt_reason"];

    state.receiptAcknowledged = values["receipt_acknowledged"] == "1";
    state.dispatchState = values["dispatch_state"];
    state.resultPresent = values["result_present"] == "1";
    state.resultAcknowledged = values["result_acknowledged"] == "1";
    if (nativeState)
    {
        state.nativeCapabilityEvidence = values["native_capability_evidence"];
        state.pluginInstanceEpoch = values["plugin_instance_epoch"];
        state.probeNonce = values["probe_nonce"];
        state.nativeExecutionSequence = nativeSequence;
        state.nativeReceiptEvidence = values["native_receipt_evidence"];
        state.nativeResultEvidence = values["native_result_evidence"];
        state.nativeReadbackEvidence = values["native_readback_evidence"];
        if (!backendAgentCommandSafeText(state.nativeCapabilityEvidence, 4096) ||
            !backendAgentCommandSafeText(state.nativeReceiptEvidence, 4096) ||
            !backendAgentCommandSafeText(state.nativeResultEvidence, 4096) ||
            !backendAgentCommandSafeText(state.nativeReadbackEvidence, 4096) ||
            (!state.pluginInstanceEpoch.empty() &&
             !backendAgentCommandSafeIdentifier(state.pluginInstanceEpoch)) ||
            (!state.probeNonce.empty() &&
             !backendAgentCommandSafeIdentifier(state.probeNonce)))
        {
            reason = "command_state_invalid_native_evidence";
            return false;
        }
    }

    if (extended && !values["state_extension"].empty())
    {
        BackendAgentCommandStateExtension extension;
        if (!backendAgentCommandStateExtensionParse(
                values["state_extension"], assignment, extension, reason) ||
            !backendAgentCommandStateExtensionValidateSupported(
                extension, assignment, reason))
        {
            reason = "command_state_invalid_extension";
            return false;
        }
        state.stateExtensionPresent = true;
        state.stateExtension = extension;
    }

    if (state.resultPresent)
    {
        auto& result = state.result;
        result.commandId = assignment.commandId;
        result.requestFingerprint = assignment.requestFingerprint;
        result.jobId = assignment.jobId;
        result.attemptId = assignment.attemptId;
        result.claimEpoch = assignment.claimEpoch;
        result.backendId = assignment.backendId;
        result.agentId = assignment.agentId;
        result.agentInstanceId = assignment.agentInstanceId;
        result.backendGeneration = assignment.backendGeneration;
        result.dispatchState = state.dispatchState;
        result.verificationState = values["verification_state"];
        result.resultCategory = values["result_category"];
        result.errorCategory = values["error_category"];
        result.retryClassification = values["retry_classification"];
        result.boundedDiagnostics = values["bounded_diagnostics"];
        result.completedAt = static_cast<std::int64_t>(completed);
        if (!backendAgentCommandValidResult(result))
        {
            reason = "command_state_invalid_result";
            return false;
        }
    }
    if (nativeState && assignment.commandType == "vdr.native.probe" &&
        assignment.payloadVersion == 2 && !state.resultPresent &&
        state.receiptAcknowledged)
    {
        state.receiptAcknowledged = false;
    }
    reason = "command_state_loaded";
    return true;
}

bool persist(const std::string& path, const LocalState& state, std::string& reason)
{
    const auto& assignment = state.assignment;
    const auto& receipt = state.receipt;
    const auto& result = state.result;
    std::string stateExtension;
    if (state.stateExtensionPresent)
    {
        if (!backendAgentCommandStateExtensionValidateSupported(
                state.stateExtension, assignment, reason))
        {
            reason = "command_state_invalid_extension";
            return false;
        }
        stateExtension = backendAgentCommandStateExtensionSerialize(
            state.stateExtension, assignment, reason);
        if (stateExtension.empty())
        {
            reason = "command_state_invalid_extension";
            return false;
        }
    }

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
        << "receipt_category=" << receipt.receiptCategory << "\n"
        << "received_at=" << receipt.receivedAt << "\n"
        << "receipt_reason=" << receipt.reasonCode << "\n"
        << "receipt_acknowledged=" << boolText(state.receiptAcknowledged) << "\n"
        << "dispatch_state=" << state.dispatchState << "\n"
        << "result_present=" << boolText(state.resultPresent) << "\n"
        << "result_acknowledged=" << boolText(state.resultAcknowledged) << "\n"
        << "verification_state="
        << (state.resultPresent ? result.verificationState : "") << "\n"
        << "result_category="
        << (state.resultPresent ? result.resultCategory : "") << "\n"
        << "error_category="
        << (state.resultPresent ? result.errorCategory : "") << "\n"
        << "retry_classification="
        << (state.resultPresent ? result.retryClassification : "") << "\n"
        << "bounded_diagnostics="
        << (state.resultPresent ? result.boundedDiagnostics : "") << "\n"
        << "completed_at="
        << (state.resultPresent ? result.completedAt : 0) << "\n"
        << "native_capability_evidence=" << state.nativeCapabilityEvidence << "\n"
        << "plugin_instance_epoch=" << state.pluginInstanceEpoch << "\n"
        << "probe_nonce=" << state.probeNonce << "\n"
        << "native_execution_sequence=" << state.nativeExecutionSequence << "\n"
        << "native_receipt_evidence=" << state.nativeReceiptEvidence << "\n"
        << "native_result_evidence=" << state.nativeResultEvidence << "\n"
        << "native_readback_evidence=" << state.nativeReadbackEvidence << "\n"
        << "state_extension=" << stateExtension << "\n";
    return writeProtected(path, output.str(), reason);
}

}

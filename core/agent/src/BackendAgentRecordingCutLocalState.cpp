#include "BackendAgentRecordingCutLocalState.h"

#include "BackendAgentRecordingCutPayload.h"

#include <algorithm>
#include <limits>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace vdrsuite::agent
{
namespace
{
constexpr std::size_t MaximumStateBytes = 64U * 1024U;

char hexDigit(unsigned value)
{
    return value < 10
        ? static_cast<char>('0' + value)
        : static_cast<char>('a' + value - 10);
}

std::string hex(const std::string& value)
{
    std::string encoded;
    encoded.reserve(value.size() * 2U);
    for (unsigned char character : value)
    {
        encoded.push_back(hexDigit(character >> 4U));
        encoded.push_back(hexDigit(character & 15U));
    }
    return encoded;
}

int hexValue(char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    return -1;
}

bool unhex(const std::string& encoded, std::string& value)
{
    value.clear();
    if (encoded.size() % 2U != 0U) return false;
    value.reserve(encoded.size() / 2U);
    for (std::size_t index = 0; index < encoded.size(); index += 2U)
    {
        const int high = hexValue(encoded[index]);
        const int low = hexValue(encoded[index + 1U]);
        if (high < 0 || low < 0) return false;
        value.push_back(static_cast<char>((high << 4) | low));
    }
    return true;
}

bool number(const std::string& value, std::uint64_t& parsed)
{
    if (value.empty()) return false;
    parsed = 0;
    for (unsigned char character : value)
    {
        if (character < '0' || character > '9') return false;
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (parsed >
            (static_cast<std::uint64_t>(
                 std::numeric_limits<std::int64_t>::max()) - digit) /
                10U)
        {
            return false;
        }
        parsed = parsed * 10U + digit;
    }
    return true;
}

const std::vector<std::string>& keys()
{
    static const std::vector<std::string> value = {
        "schema", "phase", "command_id", "request_fingerprint_hex",
        "operation_id", "payload_hex", "job_id", "attempt_id",
        "claim_epoch", "agent_id", "agent_instance_id",
        "local_starting_persisted_at", "outcome", "dispatch_started_at",
        "completed_at", "evidence_reference_hex",
    };
    return value;
}

bool exactKeys(const std::map<std::string, std::string>& values)
{
    if (values.size() != keys().size()) return false;
    for (const auto& key : keys())
        if (values.count(key) != 1U) return false;
    return true;
}

const char* outcomeName(BackendAgentRecordingCutOutcomeCategory outcome)
{
    switch (outcome)
    {
        case BackendAgentRecordingCutOutcomeCategory::rejectedWithoutEffect:
            return "rejected_without_effect";
        case BackendAgentRecordingCutOutcomeCategory::acceptedUnverified:
            return "accepted_unverified";
        case BackendAgentRecordingCutOutcomeCategory::outcomeUnknown:
            return "outcome_unknown";
    }
    return "invalid";
}

bool parseOutcome(
    const std::string& value,
    BackendAgentRecordingCutOutcomeCategory& outcome)
{
    if (value == "rejected_without_effect")
        outcome = BackendAgentRecordingCutOutcomeCategory::rejectedWithoutEffect;
    else if (value == "accepted_unverified")
        outcome = BackendAgentRecordingCutOutcomeCategory::acceptedUnverified;
    else if (value == "outcome_unknown")
        outcome = BackendAgentRecordingCutOutcomeCategory::outcomeUnknown;
    else
        return false;
    return true;
}

bool emptyEvidence(const BackendAgentRecordingCutEvidence& evidence)
{
    return evidence.commandId.empty() && evidence.requestFingerprint.empty() &&
        evidence.operationId.empty() && evidence.operationRevision.empty() &&
        evidence.jobId.empty() && evidence.attemptId.empty() &&
        evidence.claimEpoch == 0 && evidence.backendId.empty() &&
        evidence.agentId.empty() && evidence.agentInstanceId.empty() &&
        evidence.backendGeneration == 0 && evidence.providerInstanceEpoch.empty() &&
        evidence.localStartingPersistedAt == 0 && evidence.dispatchStartedAt == 0 &&
        evidence.completedAt == 0 && evidence.evidenceReference.empty();
}

BackendAgentRecordingCutPayload payloadFor(
    const BackendAgentRecordingCutCommand& command)
{
    BackendAgentRecordingCutPayload payload;
    payload.operationRevision = command.operationRevision;
    payload.recordingKey = command.recordingKey;
    payload.expectedMarksRevision = command.expectedMarksRevision;
    payload.backendId = command.backendId;
    payload.backendGeneration = command.backendGeneration;
    payload.controlPlaneClaimedAt = command.controlPlaneClaimedAt;
    payload.localProviderSelection = command.localProviderSelection;
    return payload;
}

BackendAgentRecordingCutCommand commandFor(
    const std::string& commandId,
    const std::string& requestFingerprint,
    const std::string& operationId,
    const std::string& jobId,
    const std::string& attemptId,
    std::uint64_t claimEpoch,
    const std::string& agentId,
    const std::string& agentInstanceId,
    const BackendAgentRecordingCutPayload& payload)
{
    BackendAgentRecordingCutCommand command;
    command.commandId = commandId;
    command.requestFingerprint = requestFingerprint;
    command.operationId = operationId;
    command.operationRevision = payload.operationRevision;
    command.recordingKey = payload.recordingKey;
    command.expectedMarksRevision = payload.expectedMarksRevision;
    command.jobId = jobId;
    command.attemptId = attemptId;
    command.claimEpoch = claimEpoch;
    command.backendId = payload.backendId;
    command.agentId = agentId;
    command.agentInstanceId = agentInstanceId;
    command.backendGeneration = payload.backendGeneration;
    command.controlPlaneClaimedAt = payload.controlPlaneClaimedAt;
    command.localProviderSelection = payload.localProviderSelection;
    return command;
}

BackendAgentRecordingCutEvidence evidenceFor(
    const BackendAgentRecordingCutCommand& command,
    std::int64_t localStartingPersistedAt,
    BackendAgentRecordingCutOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    std::string evidenceReference)
{
    BackendAgentRecordingCutEvidence evidence;
    evidence.commandId = command.commandId;
    evidence.requestFingerprint = command.requestFingerprint;
    evidence.operationId = command.operationId;
    evidence.operationRevision = command.operationRevision;
    evidence.jobId = command.jobId;
    evidence.attemptId = command.attemptId;
    evidence.claimEpoch = command.claimEpoch;
    evidence.backendId = command.backendId;
    evidence.agentId = command.agentId;
    evidence.agentInstanceId = command.agentInstanceId;
    evidence.backendGeneration = command.backendGeneration;
    evidence.providerInstanceEpoch = command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = localStartingPersistedAt;
    evidence.outcome = outcome;
    evidence.dispatchStartedAt = dispatchStartedAt;
    evidence.completedAt = completedAt;
    evidence.evidenceReference = std::move(evidenceReference);
    return evidence;
}

bool parseLines(
    const std::string& encoded,
    std::map<std::string, std::string>& values)
{
    values.clear();
    if (encoded.empty() || encoded.size() > MaximumStateBytes) return false;
    std::size_t start = 0;
    while (start < encoded.size())
    {
        const std::size_t end = encoded.find('\n', start);
        const std::string line = encoded.substr(
            start, end == std::string::npos ? std::string::npos : end - start);
        if (line.empty()) return false;
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos || equals == 0) return false;
        if (!values.emplace(line.substr(0, equals), line.substr(equals + 1)).second)
            return false;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return exactKeys(values);
}

}

bool backendAgentRecordingCutCommandFromAssignment(
    const BackendAgentCommandAssignment& assignment,
    BackendAgentRecordingCutCommand& command,
    std::string& reasonCode)
{
    if (!backendAgentCommandValidAssignment(assignment) ||
        assignment.commandType != kBackendAgentRecordingCutCommandType ||
        assignment.payloadVersion != kBackendAgentRecordingCutPayloadVersion ||
        assignment.verificationPolicy != "readback_required")
    {
        reasonCode = "invalid_recording_cut_assignment";
        return false;
    }

    BackendAgentRecordingCutPayload payload;
    if (!backendAgentRecordingCutParsePayload(assignment.payload, payload, reasonCode) ||
        payload.backendId != assignment.backendId ||
        payload.backendGeneration != assignment.backendGeneration)
    {
        reasonCode = "invalid_recording_cut_assignment_payload";
        return false;
    }

    auto candidate = commandFor(
        assignment.commandId, assignment.requestFingerprint,
        assignment.operationId, assignment.jobId, assignment.attemptId,
        assignment.claimEpoch, assignment.agentId, assignment.agentInstanceId,
        payload);
    if (!backendAgentRecordingCutValidCommand(candidate, reasonCode))
    {
        reasonCode = "invalid_recording_cut_command_envelope";
        return false;
    }

    command = std::move(candidate);
    reasonCode.clear();
    return true;
}

bool backendAgentRecordingCutPrepareLocalStarting(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t now,
    BackendAgentRecordingCutLocalState& state,
    std::string& reasonCode)
{
    BackendAgentRecordingCutCommand command;
    if (!backendAgentRecordingCutCommandFromAssignment(
            assignment, command, reasonCode)) return false;
    if (now < command.controlPlaneClaimedAt || now > assignment.deadline)
    {
        reasonCode = "recording_cut_starting_time_fenced";
        return false;
    }

    BackendAgentRecordingCutLocalState candidate;
    candidate.command = std::move(command);
    candidate.localStartingPersistedAt = now;
    if (!backendAgentRecordingCutLocalStateValid(candidate, reasonCode)) return false;

    state = std::move(candidate);
    reasonCode = "recording_cut_starting_ready_for_durable_persist";
    return true;
}

bool backendAgentRecordingCutLocalStateValid(
    const BackendAgentRecordingCutLocalState& state,
    std::string& reasonCode)
{
    if (state.schemaVersion != 1 ||
        !backendAgentRecordingCutValidCommand(state.command, reasonCode) ||
        state.localStartingPersistedAt < state.command.controlPlaneClaimedAt ||
        state.localStartingPersistedAt <= 0)
    {
        reasonCode = "invalid_recording_cut_local_state";
        return false;
    }

    if (state.phase == BackendAgentRecordingCutLocalPhase::starting)
    {
        if (!emptyEvidence(state.evidence))
        {
            reasonCode = "starting_recording_cut_has_evidence";
            return false;
        }
        reasonCode.clear();
        return true;
    }

    if (state.phase != BackendAgentRecordingCutLocalPhase::completed ||
        !backendAgentRecordingCutEvidenceMatches(
            state.evidence, state.command, reasonCode) ||
        state.evidence.localStartingPersistedAt != state.localStartingPersistedAt)
    {
        reasonCode = "invalid_recording_cut_completed_state";
        return false;
    }

    reasonCode.clear();
    return true;
}

bool backendAgentRecordingCutCompleteLocalState(
    BackendAgentRecordingCutLocalState& state,
    const BackendAgentRecordingCutEvidence& evidence,
    std::string& reasonCode)
{
    if (!backendAgentRecordingCutLocalStateValid(state, reasonCode) ||
        state.phase != BackendAgentRecordingCutLocalPhase::starting ||
        evidence.localStartingPersistedAt != state.localStartingPersistedAt ||
        !backendAgentRecordingCutEvidenceMatches(evidence, state.command, reasonCode))
    {
        reasonCode = "recording_cut_completion_evidence_mismatch";
        return false;
    }

    auto candidate = state;
    candidate.phase = BackendAgentRecordingCutLocalPhase::completed;
    candidate.evidence = evidence;
    if (!backendAgentRecordingCutLocalStateValid(candidate, reasonCode)) return false;

    state = std::move(candidate);
    reasonCode = "recording_cut_completion_ready_for_durable_persist";
    return true;
}

BackendAgentRecordingCutRecoveryResult backendAgentRecordingCutRecoverLocalState(
    const BackendAgentRecordingCutLocalState& state,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::int64_t now)
{
    BackendAgentRecordingCutRecoveryResult result;
    std::string reasonCode;
    if (!backendAgentRecordingCutLocalStateValid(state, reasonCode) ||
        backendId.empty() || agentId.empty() || agentInstanceId.empty() ||
        backendGeneration == 0 || now <= 0)
    {
        result.reasonCode = "recording_cut_recovery_invalid";
        return result;
    }

    if (state.phase == BackendAgentRecordingCutLocalPhase::completed)
    {
        result.decision = BackendAgentRecordingCutRecoveryDecision::returnPersistedEvidence;
        result.evidence = state.evidence;
        result.reasonCode = "recording_cut_completed_evidence_replay";
        return result;
    }

    result.evidence = evidenceFor(
        state.command,
        state.localStartingPersistedAt,
        BackendAgentRecordingCutOutcomeCategory::outcomeUnknown,
        state.localStartingPersistedAt,
        std::max(now, state.localStartingPersistedAt),
        "local-recovery:" + state.command.commandId);
    if (!backendAgentRecordingCutEvidenceMatches(
            result.evidence, state.command, reasonCode))
    {
        result.reasonCode = "recording_cut_recovery_evidence_invalid";
        return result;
    }

    result.decision = BackendAgentRecordingCutRecoveryDecision::reconcileOnly;
    result.reasonCode =
        backendId == state.command.backendId &&
        agentId == state.command.agentId &&
        agentInstanceId == state.command.agentInstanceId &&
        backendGeneration == state.command.backendGeneration
        ? "recording_cut_starting_recovery_reconcile_only"
        : "recording_cut_starting_context_fenced_reconcile_only";
    return result;
}

std::string backendAgentRecordingCutSerializeLocalState(
    const BackendAgentRecordingCutLocalState& state,
    std::string& reasonCode)
{
    if (!backendAgentRecordingCutLocalStateValid(state, reasonCode)) return {};

    const bool completed = state.phase == BackendAgentRecordingCutLocalPhase::completed;
    const auto payload = backendAgentRecordingCutPayload(payloadFor(state.command));
    if (payload.empty())
    {
        reasonCode = "recording_cut_state_payload_invalid";
        return {};
    }

    std::ostringstream out;
    out << "schema=1\n"
        << "phase=" << (completed ? "completed" : "starting") << '\n'
        << "command_id=" << state.command.commandId << '\n'
        << "request_fingerprint_hex=" << hex(state.command.requestFingerprint) << '\n'
        << "operation_id=" << state.command.operationId << '\n'
        << "payload_hex=" << hex(payload) << '\n'
        << "job_id=" << state.command.jobId << '\n'
        << "attempt_id=" << state.command.attemptId << '\n'
        << "claim_epoch=" << state.command.claimEpoch << '\n'
        << "agent_id=" << state.command.agentId << '\n'
        << "agent_instance_id=" << state.command.agentInstanceId << '\n'
        << "local_starting_persisted_at=" << state.localStartingPersistedAt << '\n'
        << "outcome=" << (completed ? outcomeName(state.evidence.outcome) : "-") << '\n'
        << "dispatch_started_at=" << (completed ? state.evidence.dispatchStartedAt : 0) << '\n'
        << "completed_at=" << (completed ? state.evidence.completedAt : 0) << '\n'
        << "evidence_reference_hex="
        << (completed ? hex(state.evidence.evidenceReference) : std::string()) << '\n';
    const std::string encoded = out.str();
    if (encoded.size() > MaximumStateBytes)
    {
        reasonCode = "recording_cut_local_state_too_large";
        return {};
    }
    reasonCode.clear();
    return encoded;
}

bool backendAgentRecordingCutParseLocalState(
    const std::string& encoded,
    BackendAgentRecordingCutLocalState& state,
    std::string& reasonCode)
{
    std::map<std::string, std::string> values;
    if (!parseLines(encoded, values) || values["schema"] != "1" ||
        (values["phase"] != "starting" && values["phase"] != "completed"))
    {
        reasonCode = "recording_cut_local_state_malformed";
        return false;
    }

    std::string requestFingerprint;
    std::string payloadEncoded;
    std::string evidenceReference;
    std::uint64_t claimEpoch = 0;
    std::uint64_t localStarting = 0;
    std::uint64_t dispatchStarted = 0;
    std::uint64_t completedAt = 0;
    if (!unhex(values["request_fingerprint_hex"], requestFingerprint) ||
        !unhex(values["payload_hex"], payloadEncoded) ||
        !number(values["claim_epoch"], claimEpoch) || claimEpoch == 0 ||
        !number(values["local_starting_persisted_at"], localStarting) ||
        localStarting == 0 ||
        !number(values["dispatch_started_at"], dispatchStarted) ||
        !number(values["completed_at"], completedAt) ||
        !unhex(values["evidence_reference_hex"], evidenceReference))
    {
        reasonCode = "recording_cut_local_state_malformed";
        return false;
    }

    BackendAgentRecordingCutPayload payload;
    if (!backendAgentRecordingCutParsePayload(payloadEncoded, payload, reasonCode))
    {
        reasonCode = "recording_cut_local_state_payload_invalid";
        return false;
    }

    BackendAgentRecordingCutLocalState candidate;
    candidate.phase = values["phase"] == "completed"
        ? BackendAgentRecordingCutLocalPhase::completed
        : BackendAgentRecordingCutLocalPhase::starting;
    candidate.command = commandFor(
        values["command_id"], requestFingerprint, values["operation_id"],
        values["job_id"], values["attempt_id"], claimEpoch,
        values["agent_id"], values["agent_instance_id"], payload);
    candidate.localStartingPersistedAt = static_cast<std::int64_t>(localStarting);

    if (candidate.phase == BackendAgentRecordingCutLocalPhase::starting)
    {
        if (values["outcome"] != "-" || dispatchStarted != 0 ||
            completedAt != 0 || !evidenceReference.empty())
        {
            reasonCode = "recording_cut_starting_state_not_empty";
            return false;
        }
    }
    else
    {
        BackendAgentRecordingCutOutcomeCategory outcome;
        if (!parseOutcome(values["outcome"], outcome) || completedAt == 0)
        {
            reasonCode = "recording_cut_completed_state_malformed";
            return false;
        }
        candidate.evidence = evidenceFor(
            candidate.command,
            candidate.localStartingPersistedAt,
            outcome,
            static_cast<std::int64_t>(dispatchStarted),
            static_cast<std::int64_t>(completedAt),
            evidenceReference);
    }

    if (!backendAgentRecordingCutLocalStateValid(candidate, reasonCode) ||
        backendAgentRecordingCutSerializeLocalState(candidate, reasonCode) != encoded)
    {
        if (reasonCode.empty()) reasonCode = "recording_cut_local_state_not_canonical";
        return false;
    }

    state = std::move(candidate);
    reasonCode.clear();
    return true;
}

}

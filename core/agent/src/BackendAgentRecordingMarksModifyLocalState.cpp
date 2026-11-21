#include "BackendAgentRecordingMarksModifyLocalState.h"

#include "BackendAgentRecordingMarksModifyPayload.h"

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
        "schema",
        "phase",
        "command_id",
        "request_fingerprint_hex",
        "operation_id",
        "payload_hex",
        "job_id",
        "attempt_id",
        "claim_epoch",
        "agent_id",
        "agent_instance_id",
        "local_starting_persisted_at",
        "outcome",
        "dispatch_started_at",
        "completed_at",
        "evidence_reference_hex",
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

const char* outcomeName(BackendAgentRecordingMarksModifyOutcomeCategory outcome)
{
    switch (outcome)
    {
        case BackendAgentRecordingMarksModifyOutcomeCategory::rejectedWithoutEffect:
            return "rejected_without_effect";
        case BackendAgentRecordingMarksModifyOutcomeCategory::acceptedUnverified:
            return "accepted_unverified";
        case BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown:
            return "outcome_unknown";
    }
    return "invalid";
}

bool parseOutcome(
    const std::string& value,
    BackendAgentRecordingMarksModifyOutcomeCategory& outcome)
{
    if (value == "rejected_without_effect")
        outcome = BackendAgentRecordingMarksModifyOutcomeCategory::rejectedWithoutEffect;
    else if (value == "accepted_unverified")
        outcome = BackendAgentRecordingMarksModifyOutcomeCategory::acceptedUnverified;
    else if (value == "outcome_unknown")
        outcome = BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown;
    else
        return false;
    return true;
}

bool emptyEvidence(const BackendAgentRecordingMarksModifyEvidence& evidence)
{
    return evidence.commandId.empty() &&
        evidence.requestFingerprint.empty() &&
        evidence.operationId.empty() &&
        evidence.operationRevision.empty() &&
        evidence.jobId.empty() &&
        evidence.attemptId.empty() &&
        evidence.claimEpoch == 0 &&
        evidence.backendId.empty() &&
        evidence.agentId.empty() &&
        evidence.agentInstanceId.empty() &&
        evidence.backendGeneration == 0 &&
        evidence.providerInstanceEpoch.empty() &&
        evidence.localStartingPersistedAt == 0 &&
        evidence.dispatchStartedAt == 0 &&
        evidence.completedAt == 0 &&
        evidence.evidenceReference.empty();
}

BackendAgentRecordingMarksModifyPayload payloadFor(
    const BackendAgentRecordingMarksModifyCommand& command)
{
    BackendAgentRecordingMarksModifyPayload payload;
    payload.kind = command.kind;
    payload.operationRevision = command.operationRevision;
    payload.recordingKey = command.recordingKey;
    payload.expectedMarksRevision = command.expectedMarksRevision;
    payload.sourceFrame = command.sourceFrame;
    payload.targetFrame = command.targetFrame;
    payload.replacementFrames = command.replacementFrames;
    payload.backendId = command.backendId;
    payload.backendGeneration = command.backendGeneration;
    payload.controlPlaneClaimedAt = command.controlPlaneClaimedAt;
    payload.localProviderSelection = command.localProviderSelection;
    return payload;
}

BackendAgentRecordingMarksModifyCommand commandFor(
    const std::string& commandId,
    const std::string& requestFingerprint,
    const std::string& operationId,
    const std::string& jobId,
    const std::string& attemptId,
    std::uint64_t claimEpoch,
    const std::string& agentId,
    const std::string& agentInstanceId,
    const BackendAgentRecordingMarksModifyPayload& payload)
{
    BackendAgentRecordingMarksModifyCommand command;
    command.kind = payload.kind;
    command.commandId = commandId;
    command.requestFingerprint = requestFingerprint;
    command.operationId = operationId;
    command.operationRevision = payload.operationRevision;
    command.recordingKey = payload.recordingKey;
    command.expectedMarksRevision = payload.expectedMarksRevision;
    command.sourceFrame = payload.sourceFrame;
    command.targetFrame = payload.targetFrame;
    command.replacementFrames = payload.replacementFrames;
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

BackendAgentRecordingMarksModifyEvidence evidenceFor(
    const BackendAgentRecordingMarksModifyCommand& command,
    std::int64_t localStartingPersistedAt,
    BackendAgentRecordingMarksModifyOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    std::string evidenceReference)
{
    BackendAgentRecordingMarksModifyEvidence evidence;
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
    evidence.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = localStartingPersistedAt;
    evidence.outcome = outcome;
    evidence.dispatchStartedAt = dispatchStartedAt;
    evidence.completedAt = completedAt;
    evidence.evidenceReference = std::move(evidenceReference);
    return evidence;
}

}

bool backendAgentRecordingMarksModifyCommandFromAssignment(
    const BackendAgentCommandAssignment& assignment,
    BackendAgentRecordingMarksModifyCommand& command,
    std::string& reasonCode)
{
    if (!backendAgentCommandValidAssignment(assignment) ||
        assignment.commandType != kBackendAgentRecordingMarksModifyCommandType ||
        assignment.payloadVersion !=
            kBackendAgentRecordingMarksModifyPayloadVersion ||
        assignment.verificationPolicy != "readback_required")
    {
        reasonCode = "invalid_recording_marks_modify_assignment";
        return false;
    }

    BackendAgentRecordingMarksModifyPayload payload;
    if (!backendAgentRecordingMarksModifyParsePayload(
            assignment.payload, payload, reasonCode) ||
        payload.backendId != assignment.backendId ||
        payload.backendGeneration != assignment.backendGeneration)
    {
        reasonCode = "invalid_recording_marks_modify_assignment_payload";
        return false;
    }

    auto candidate = commandFor(
        assignment.commandId,
        assignment.requestFingerprint,
        assignment.operationId,
        assignment.jobId,
        assignment.attemptId,
        assignment.claimEpoch,
        assignment.agentId,
        assignment.agentInstanceId,
        payload);
    if (!backendAgentRecordingMarksModifyValidCommand(candidate, reasonCode))
    {
        reasonCode = "invalid_recording_marks_modify_command_envelope";
        return false;
    }

    command = std::move(candidate);
    reasonCode.clear();
    return true;
}

bool backendAgentRecordingMarksModifyPrepareLocalStarting(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t now,
    BackendAgentRecordingMarksModifyLocalState& state,
    std::string& reasonCode)
{
    BackendAgentRecordingMarksModifyCommand command;
    if (!backendAgentRecordingMarksModifyCommandFromAssignment(
            assignment, command, reasonCode))
    {
        return false;
    }
    if (now < command.controlPlaneClaimedAt || now > assignment.deadline)
    {
        reasonCode = "recording_marks_modify_starting_time_fenced";
        return false;
    }

    BackendAgentRecordingMarksModifyLocalState candidate;
    candidate.command = std::move(command);
    candidate.localStartingPersistedAt = now;
    if (!backendAgentRecordingMarksModifyLocalStateValid(
            candidate, reasonCode))
    {
        return false;
    }

    state = std::move(candidate);
    reasonCode = "recording_marks_modify_starting_ready_for_durable_persist";
    return true;
}

bool backendAgentRecordingMarksModifyLocalStateValid(
    const BackendAgentRecordingMarksModifyLocalState& state,
    std::string& reasonCode)
{
    if (state.schemaVersion != 1 ||
        !backendAgentRecordingMarksModifyValidCommand(
            state.command, reasonCode) ||
        state.localStartingPersistedAt < state.command.controlPlaneClaimedAt ||
        state.localStartingPersistedAt <= 0)
    {
        reasonCode = "invalid_recording_marks_modify_local_state";
        return false;
    }

    if (state.phase == BackendAgentRecordingMarksModifyLocalPhase::starting)
    {
        if (!emptyEvidence(state.evidence))
        {
            reasonCode = "starting_recording_marks_modify_has_evidence";
            return false;
        }
        reasonCode.clear();
        return true;
    }

    if (state.phase != BackendAgentRecordingMarksModifyLocalPhase::completed ||
        !backendAgentRecordingMarksModifyEvidenceMatches(
            state.evidence, state.command, reasonCode) ||
        state.evidence.localStartingPersistedAt !=
            state.localStartingPersistedAt)
    {
        reasonCode = "invalid_recording_marks_modify_completed_state";
        return false;
    }

    reasonCode.clear();
    return true;
}

bool backendAgentRecordingMarksModifyCompleteLocalState(
    BackendAgentRecordingMarksModifyLocalState& state,
    const BackendAgentRecordingMarksModifyEvidence& evidence,
    std::string& reasonCode)
{
    if (!backendAgentRecordingMarksModifyLocalStateValid(state, reasonCode) ||
        state.phase != BackendAgentRecordingMarksModifyLocalPhase::starting ||
        evidence.localStartingPersistedAt != state.localStartingPersistedAt ||
        !backendAgentRecordingMarksModifyEvidenceMatches(
            evidence, state.command, reasonCode))
    {
        reasonCode = "recording_marks_modify_completion_evidence_mismatch";
        return false;
    }

    auto candidate = state;
    candidate.phase = BackendAgentRecordingMarksModifyLocalPhase::completed;
    candidate.evidence = evidence;
    if (!backendAgentRecordingMarksModifyLocalStateValid(
            candidate, reasonCode))
    {
        return false;
    }

    state = std::move(candidate);
    reasonCode = "recording_marks_modify_completion_ready_for_durable_persist";
    return true;
}

BackendAgentRecordingMarksModifyRecoveryResult
backendAgentRecordingMarksModifyRecoverLocalState(
    const BackendAgentRecordingMarksModifyLocalState& state,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::int64_t now)
{
    BackendAgentRecordingMarksModifyRecoveryResult result;
    std::string reasonCode;
    if (!backendAgentRecordingMarksModifyLocalStateValid(state, reasonCode) ||
        backendId.empty() ||
        agentId.empty() ||
        agentInstanceId.empty() ||
        backendGeneration == 0 ||
        now <= 0)
    {
        result.reasonCode = "recording_marks_modify_recovery_invalid";
        return result;
    }

    if (state.phase == BackendAgentRecordingMarksModifyLocalPhase::completed)
    {
        result.decision =
            BackendAgentRecordingMarksModifyRecoveryDecision::returnPersistedEvidence;
        result.evidence = state.evidence;
        result.reasonCode = "recording_marks_modify_completed_evidence_replay";
        return result;
    }

    result.evidence = evidenceFor(
        state.command,
        state.localStartingPersistedAt,
        BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown,
        state.localStartingPersistedAt,
        std::max(now, state.localStartingPersistedAt),
        "local-recovery:" + state.command.commandId);
    if (!backendAgentRecordingMarksModifyEvidenceMatches(
            result.evidence, state.command, reasonCode))
    {
        result.reasonCode = "recording_marks_modify_recovery_evidence_invalid";
        return result;
    }

    result.decision =
        BackendAgentRecordingMarksModifyRecoveryDecision::reconcileOnly;
    result.reasonCode =
        backendId == state.command.backendId &&
        agentId == state.command.agentId &&
        agentInstanceId == state.command.agentInstanceId &&
        backendGeneration == state.command.backendGeneration
        ? "recording_marks_modify_starting_recovery_reconcile_only"
        : "recording_marks_modify_starting_context_fenced_reconcile_only";
    return result;
}

std::string backendAgentRecordingMarksModifySerializeLocalState(
    const BackendAgentRecordingMarksModifyLocalState& state,
    std::string& reasonCode)
{
    if (!backendAgentRecordingMarksModifyLocalStateValid(state, reasonCode))
        return {};

    const bool completed =
        state.phase == BackendAgentRecordingMarksModifyLocalPhase::completed;
    const auto payload = backendAgentRecordingMarksModifyPayload(
        payloadFor(state.command));
    if (payload.empty())
    {
        reasonCode = "recording_marks_modify_state_payload_invalid";
        return {};
    }

    std::ostringstream out;
    out << "schema=1\n"
        << "phase=" << (completed ? "completed" : "starting") << '\n'
        << "command_id=" << state.command.commandId << '\n'
        << "request_fingerprint_hex="
        << hex(state.command.requestFingerprint) << '\n'
        << "operation_id=" << state.command.operationId << '\n'
        << "payload_hex=" << hex(payload) << '\n'
        << "job_id=" << state.command.jobId << '\n'
        << "attempt_id=" << state.command.attemptId << '\n'
        << "claim_epoch=" << state.command.claimEpoch << '\n'
        << "agent_id=" << state.command.agentId << '\n'
        << "agent_instance_id=" << state.command.agentInstanceId << '\n'
        << "local_starting_persisted_at="
        << state.localStartingPersistedAt << '\n'
        << "outcome="
        << (completed ? outcomeName(state.evidence.outcome) : "none") << '\n'
        << "dispatch_started_at="
        << (completed ? state.evidence.dispatchStartedAt : 0) << '\n'
        << "completed_at="
        << (completed ? state.evidence.completedAt : 0) << '\n'
        << "evidence_reference_hex="
        << (completed ? hex(state.evidence.evidenceReference) : std::string{})
        << '\n';

    const auto encoded = out.str();
    if (encoded.size() > MaximumStateBytes)
    {
        reasonCode = "recording_marks_modify_state_too_large";
        return {};
    }

    reasonCode.clear();
    return encoded;
}

bool backendAgentRecordingMarksModifyParseLocalState(
    const std::string& encoded,
    BackendAgentRecordingMarksModifyLocalState& state,
    std::string& reasonCode)
{
    if (encoded.empty() || encoded.size() > MaximumStateBytes)
    {
        reasonCode = "invalid_recording_marks_modify_state";
        return false;
    }

    std::map<std::string, std::string> values;
    std::istringstream input(encoded);
    std::string line;
    while (std::getline(input, line))
    {
        const auto separator = line.find('=');
        if (separator == std::string::npos || separator == 0 ||
            !values.emplace(
                line.substr(0, separator),
                line.substr(separator + 1)).second)
        {
            reasonCode = "invalid_recording_marks_modify_state";
            return false;
        }
    }

    if (!exactKeys(values) || values["schema"] != "1")
    {
        reasonCode = "invalid_recording_marks_modify_state_schema";
        return false;
    }

    BackendAgentRecordingMarksModifyLocalState candidate;
    if (values["phase"] == "completed")
        candidate.phase = BackendAgentRecordingMarksModifyLocalPhase::completed;
    else if (values["phase"] != "starting")
    {
        reasonCode = "invalid_recording_marks_modify_state_phase";
        return false;
    }

    std::string requestFingerprint;
    std::string payloadText;
    std::string evidenceReference;
    BackendAgentRecordingMarksModifyPayload payload;
    std::uint64_t claimEpoch = 0;
    std::uint64_t localStartingPersistedAt = 0;
    std::uint64_t dispatchStartedAt = 0;
    std::uint64_t completedAt = 0;
    if (!unhex(values["request_fingerprint_hex"], requestFingerprint) ||
        !unhex(values["payload_hex"], payloadText) ||
        !unhex(values["evidence_reference_hex"], evidenceReference) ||
        !backendAgentRecordingMarksModifyParsePayload(
            payloadText, payload, reasonCode) ||
        !number(values["claim_epoch"], claimEpoch) ||
        !number(
            values["local_starting_persisted_at"],
            localStartingPersistedAt) ||
        !number(values["dispatch_started_at"], dispatchStartedAt) ||
        !number(values["completed_at"], completedAt))
    {
        reasonCode = "invalid_recording_marks_modify_state_encoding";
        return false;
    }

    candidate.command = commandFor(
        values["command_id"],
        requestFingerprint,
        values["operation_id"],
        values["job_id"],
        values["attempt_id"],
        claimEpoch,
        values["agent_id"],
        values["agent_instance_id"],
        payload);
    candidate.localStartingPersistedAt =
        static_cast<std::int64_t>(localStartingPersistedAt);

    if (candidate.phase == BackendAgentRecordingMarksModifyLocalPhase::starting)
    {
        if (values["outcome"] != "none" ||
            dispatchStartedAt != 0 ||
            completedAt != 0 ||
            !evidenceReference.empty())
        {
            reasonCode = "invalid_recording_marks_modify_starting_encoding";
            return false;
        }
    }
    else
    {
        BackendAgentRecordingMarksModifyOutcomeCategory outcome;
        if (!parseOutcome(values["outcome"], outcome))
        {
            reasonCode = "invalid_recording_marks_modify_outcome";
            return false;
        }
        candidate.evidence = evidenceFor(
            candidate.command,
            candidate.localStartingPersistedAt,
            outcome,
            static_cast<std::int64_t>(dispatchStartedAt),
            static_cast<std::int64_t>(completedAt),
            evidenceReference);
    }

    if (!backendAgentRecordingMarksModifyLocalStateValid(
            candidate, reasonCode))
    {
        return false;
    }

    state = std::move(candidate);
    reasonCode.clear();
    return true;
}

}

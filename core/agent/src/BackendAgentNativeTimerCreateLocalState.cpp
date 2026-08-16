#include "BackendAgentNativeTimerCreateLocalState.h"

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerCreatePayload.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::agent
{
namespace
{
constexpr const char* kPrefix = "native-timer-create-local-state/1|";
constexpr std::size_t kFieldCountStarting = 13;
constexpr std::size_t kFieldCountCompleted = 17;
constexpr std::size_t kMaximumEncodedBytes = 32U * 1024U;

void appendField(std::string& output, const std::string& value)
{
    output += std::to_string(value.size());
    output += ':';
    output += value;
    output += '|';
}

void appendUnsigned(std::string& output, std::uint64_t value)
{
    appendField(output, std::to_string(value));
}

bool readField(const std::string& input, std::size_t& position, std::string& value)
{
    if (position >= input.size()) return false;
    std::size_t length = 0;
    bool digitSeen = false;
    while (position < input.size() && input[position] != ':')
    {
        const char character = input[position++];
        if (character < '0' || character > '9') return false;
        digitSeen = true;
        const std::size_t digit = static_cast<std::size_t>(character - '0');
        if (length > (std::numeric_limits<std::size_t>::max() - digit) / 10)
            return false;
        length = length * 10 + digit;
    }
    if (!digitSeen || position >= input.size() || input[position] != ':') return false;
    ++position;
    if (length > input.size() - position) return false;
    value = input.substr(position, length);
    position += length;
    if (position >= input.size() || input[position] != '|') return false;
    ++position;
    return true;
}

bool parseUnsigned(const std::string& token, std::uint64_t& value, bool positive = true)
{
    if (token.empty()) return false;
    value = 0;
    for (char character : token)
    {
        if (character < '0' || character > '9') return false;
        const std::uint64_t digit = static_cast<std::uint64_t>(character - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return !positive || value > 0;
}

bool parseTime(const std::string& token, std::int64_t& value, bool positive = true)
{
    std::uint64_t parsed = 0;
    if (!parseUnsigned(token, parsed, positive) ||
        parsed > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return false;
    value = static_cast<std::int64_t>(parsed);
    return true;
}

std::string phaseName(BackendAgentNativeTimerCreateLocalPhase phase)
{
    return phase == BackendAgentNativeTimerCreateLocalPhase::starting
        ? "starting" : "completed";
}

bool parsePhase(
    const std::string& token,
    BackendAgentNativeTimerCreateLocalPhase& phase)
{
    if (token == "starting")
    {
        phase = BackendAgentNativeTimerCreateLocalPhase::starting;
        return true;
    }
    if (token == "completed")
    {
        phase = BackendAgentNativeTimerCreateLocalPhase::completed;
        return true;
    }
    return false;
}

std::string outcomeName(BackendAgentNativeTimerCreateOutcomeCategory outcome)
{
    switch (outcome)
    {
        case BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect:
            return "rejected_without_effect";
        case BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified:
            return "accepted_unverified";
        case BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown:
            return "outcome_unknown";
    }
    return {};
}

bool parseOutcome(
    const std::string& token,
    BackendAgentNativeTimerCreateOutcomeCategory& outcome)
{
    if (token == "rejected_without_effect")
    {
        outcome = BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect;
        return true;
    }
    if (token == "accepted_unverified")
    {
        outcome = BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified;
        return true;
    }
    if (token == "outcome_unknown")
    {
        outcome = BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown;
        return true;
    }
    return false;
}

BackendAgentNativeTimerCreatePayload payloadFromCommand(
    const BackendAgentNativeTimerCreateCommand& command)
{
    BackendAgentNativeTimerCreatePayload payload;
    payload.operationRevision = command.operationRevision;
    payload.timerAssignmentId = command.timerAssignmentId;
    payload.expectedAssignmentRevision = command.expectedAssignmentRevision;
    payload.expectedIntentRevision = command.expectedIntentRevision;
    payload.assignmentEpoch = command.assignmentEpoch;
    payload.nativeTimerBindingId = command.nativeTimerBindingId;
    payload.controlPlaneClaimedAt = command.controlPlaneClaimedAt;
    payload.expectedSpecificationFingerprint = command.expectedSpecificationFingerprint;
    payload.specification = command.specification;
    payload.localProviderSelection = command.localProviderSelection;
    return payload;
}

BackendAgentNativeTimerCreateCommand commandFromEnvelope(
    const std::vector<std::string>& fields,
    const BackendAgentNativeTimerCreatePayload& payload,
    bool& ok)
{
    BackendAgentNativeTimerCreateCommand command;
    ok = fields.size() >= kFieldCountStarting;
    if (!ok) return command;
    command.commandId = fields[2];
    command.requestFingerprint = fields[3];
    command.operationId = fields[4];
    command.jobId = fields[5];
    command.attemptId = fields[6];
    ok = parseUnsigned(fields[7], command.claimEpoch);
    command.backendId = fields[8];
    command.agentId = fields[9];
    command.agentInstanceId = fields[10];
    ok = ok && parseUnsigned(fields[11], command.backendGeneration);
    command.operationRevision = payload.operationRevision;
    command.timerAssignmentId = payload.timerAssignmentId;
    command.expectedAssignmentRevision = payload.expectedAssignmentRevision;
    command.expectedIntentRevision = payload.expectedIntentRevision;
    command.assignmentEpoch = payload.assignmentEpoch;
    command.nativeTimerBindingId = payload.nativeTimerBindingId;
    command.expectedSpecificationFingerprint = payload.expectedSpecificationFingerprint;
    command.controlPlaneClaimedAt = payload.controlPlaneClaimedAt;
    command.specification = payload.specification;
    command.localProviderSelection = payload.localProviderSelection;
    return command;
}

BackendAgentNativeTimerCreateEvidence evidenceFor(
    const BackendAgentNativeTimerCreateCommand& command,
    std::int64_t localStartingPersistedAt,
    BackendAgentNativeTimerCreateOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    const std::string& reference)
{
    BackendAgentNativeTimerCreateEvidence evidence;
    evidence.commandId = command.commandId;
    evidence.requestFingerprint = command.requestFingerprint;
    evidence.operationId = command.operationId;
    evidence.operationRevision = command.operationRevision;
    evidence.timerAssignmentId = command.timerAssignmentId;
    evidence.nativeTimerBindingId = command.nativeTimerBindingId;
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
    evidence.evidenceReference = reference;
    return evidence;
}
}

bool backendAgentNativeTimerCreateCommandFromAssignment(
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerCreateCommand& command,
    std::string& reasonCode)
{
    if (!backendAgentCommandValidAssignment(assignment) ||
        assignment.commandType != kBackendAgentNativeTimerCreateCommandType ||
        assignment.payloadVersion != kBackendAgentNativeTimerCreatePayloadVersion ||
        assignment.verificationPolicy != "readback_required")
    {
        reasonCode = "invalid_native_timer_create_assignment";
        return false;
    }

    BackendAgentNativeTimerCreatePayload payload;
    if (!backendAgentNativeTimerCreateParsePayload(
            assignment.payload, payload, reasonCode))
    {
        reasonCode = "invalid_native_timer_create_assignment_payload";
        return false;
    }

    BackendAgentNativeTimerCreateCommand candidate;
    candidate.commandId = assignment.commandId;
    candidate.requestFingerprint = assignment.requestFingerprint;
    candidate.operationId = assignment.operationId;
    candidate.operationRevision = payload.operationRevision;
    candidate.timerAssignmentId = payload.timerAssignmentId;
    candidate.expectedAssignmentRevision = payload.expectedAssignmentRevision;
    candidate.expectedIntentRevision = payload.expectedIntentRevision;
    candidate.assignmentEpoch = payload.assignmentEpoch;
    candidate.nativeTimerBindingId = payload.nativeTimerBindingId;
    candidate.expectedSpecificationFingerprint = payload.expectedSpecificationFingerprint;
    candidate.jobId = assignment.jobId;
    candidate.attemptId = assignment.attemptId;
    candidate.claimEpoch = assignment.claimEpoch;
    candidate.backendId = assignment.backendId;
    candidate.agentId = assignment.agentId;
    candidate.agentInstanceId = assignment.agentInstanceId;
    candidate.backendGeneration = assignment.backendGeneration;
    candidate.controlPlaneClaimedAt = payload.controlPlaneClaimedAt;
    candidate.specification = payload.specification;
    candidate.localProviderSelection = payload.localProviderSelection;
    if (!backendAgentNativeTimerCreateValidCommand(candidate, reasonCode))
    {
        reasonCode = "invalid_native_timer_create_assignment_contract";
        return false;
    }
    command = std::move(candidate);
    reasonCode.clear();
    return true;
}

bool backendAgentNativeTimerCreatePrepareLocalStarting(
    const BackendAgentNativeTimerCreateCommand& command,
    std::int64_t persistedAt,
    BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode)
{
    std::string commandReason;
    if (!backendAgentNativeTimerCreateValidCommand(command, commandReason) ||
        persistedAt <= 0 || persistedAt < command.controlPlaneClaimedAt)
    {
        reasonCode = "invalid_native_timer_create_local_starting";
        return false;
    }
    BackendAgentNativeTimerCreateLocalState candidate;
    candidate.phase = BackendAgentNativeTimerCreateLocalPhase::starting;
    candidate.command = command;
    candidate.localStartingPersistedAt = persistedAt;
    if (!backendAgentNativeTimerCreateLocalStateValid(candidate, reasonCode))
        return false;
    state = std::move(candidate);
    reasonCode.clear();
    return true;
}

bool backendAgentNativeTimerCreateLocalStateValid(
    const BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode)
{
    std::string commandReason;
    if (state.schemaVersion != 1 ||
        !backendAgentNativeTimerCreateValidCommand(state.command, commandReason) ||
        state.localStartingPersistedAt <= 0 ||
        state.localStartingPersistedAt < state.command.controlPlaneClaimedAt)
    {
        reasonCode = "invalid_native_timer_create_local_state";
        return false;
    }
    if (state.phase == BackendAgentNativeTimerCreateLocalPhase::starting)
    {
        if (state.evidence.completedAt != 0 ||
            state.evidence.dispatchStartedAt != 0 ||
            !state.evidence.evidenceReference.empty())
        {
            reasonCode = "invalid_native_timer_create_starting_evidence";
            return false;
        }
        reasonCode.clear();
        return true;
    }
    if (!backendAgentNativeTimerCreateEvidenceMatches(
            state.evidence, state.command, reasonCode) ||
        state.evidence.localStartingPersistedAt != state.localStartingPersistedAt)
    {
        reasonCode = "invalid_native_timer_create_completed_evidence";
        return false;
    }
    reasonCode.clear();
    return true;
}

bool backendAgentNativeTimerCreateCompleteLocalState(
    BackendAgentNativeTimerCreateLocalState& state,
    const BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode)
{
    if (state.phase != BackendAgentNativeTimerCreateLocalPhase::starting ||
        !backendAgentNativeTimerCreateLocalStateValid(state, reasonCode) ||
        !backendAgentNativeTimerCreateEvidenceMatches(
            evidence, state.command, reasonCode) ||
        evidence.localStartingPersistedAt != state.localStartingPersistedAt)
    {
        reasonCode = "invalid_native_timer_create_completion";
        return false;
    }
    state.phase = BackendAgentNativeTimerCreateLocalPhase::completed;
    state.evidence = evidence;
    if (!backendAgentNativeTimerCreateLocalStateValid(state, reasonCode))
        return false;
    reasonCode.clear();
    return true;
}

BackendAgentNativeTimerCreateRecoveryDecision
backendAgentNativeTimerCreateRecoverLocalState(
    const BackendAgentNativeTimerCreateLocalState& state,
    BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode)
{
    if (!backendAgentNativeTimerCreateLocalStateValid(state, reasonCode))
        return BackendAgentNativeTimerCreateRecoveryDecision::failClosed;
    if (state.phase == BackendAgentNativeTimerCreateLocalPhase::starting)
    {
        reasonCode = "native_timer_create_dispatch_may_have_occurred_reconcile_only";
        return BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly;
    }
    evidence = state.evidence;
    reasonCode = "native_timer_create_persisted_evidence_recovered";
    return BackendAgentNativeTimerCreateRecoveryDecision::returnPersistedEvidence;
}

std::string backendAgentNativeTimerCreateSerializeLocalState(
    const BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode)
{
    if (!backendAgentNativeTimerCreateLocalStateValid(state, reasonCode)) return {};
    const std::string payload = backendAgentNativeTimerCreatePayload(
        payloadFromCommand(state.command));
    if (payload.empty())
    {
        reasonCode = "invalid_native_timer_create_local_state_payload";
        return {};
    }

    std::string encoded(kPrefix);
    appendField(encoded, phaseName(state.phase));
    appendField(encoded, payload);
    appendField(encoded, state.command.commandId);
    appendField(encoded, state.command.requestFingerprint);
    appendField(encoded, state.command.operationId);
    appendField(encoded, state.command.jobId);
    appendField(encoded, state.command.attemptId);
    appendUnsigned(encoded, state.command.claimEpoch);
    appendField(encoded, state.command.backendId);
    appendField(encoded, state.command.agentId);
    appendField(encoded, state.command.agentInstanceId);
    appendUnsigned(encoded, state.command.backendGeneration);
    appendUnsigned(encoded, static_cast<std::uint64_t>(state.localStartingPersistedAt));
    if (state.phase == BackendAgentNativeTimerCreateLocalPhase::completed)
    {
        appendField(encoded, outcomeName(state.evidence.outcome));
        appendUnsigned(encoded, static_cast<std::uint64_t>(state.evidence.dispatchStartedAt));
        appendUnsigned(encoded, static_cast<std::uint64_t>(state.evidence.completedAt));
        appendField(encoded, state.evidence.evidenceReference);
    }
    if (encoded.size() > kMaximumEncodedBytes)
    {
        reasonCode = "native_timer_create_local_state_too_large";
        return {};
    }
    reasonCode.clear();
    return encoded;
}

bool backendAgentNativeTimerCreateParseLocalState(
    const std::string& encoded,
    BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode)
{
    const std::string prefix(kPrefix);
    if (encoded.compare(0, prefix.size(), prefix) != 0 ||
        encoded.size() > kMaximumEncodedBytes)
    {
        reasonCode = "invalid_native_timer_create_local_state_encoding";
        return false;
    }
    std::size_t position = prefix.size();
    std::vector<std::string> fields;
    while (position < encoded.size())
    {
        std::string field;
        if (!readField(encoded, position, field) || fields.size() >= kFieldCountCompleted)
        {
            reasonCode = "invalid_native_timer_create_local_state_encoding";
            return false;
        }
        fields.push_back(std::move(field));
    }
    if (fields.size() != kFieldCountStarting &&
        fields.size() != kFieldCountCompleted)
    {
        reasonCode = "invalid_native_timer_create_local_state_encoding";
        return false;
    }

    BackendAgentNativeTimerCreateLocalState candidate;
    if (!parsePhase(fields[0], candidate.phase) ||
        (candidate.phase == BackendAgentNativeTimerCreateLocalPhase::starting &&
         fields.size() != kFieldCountStarting) ||
        (candidate.phase == BackendAgentNativeTimerCreateLocalPhase::completed &&
         fields.size() != kFieldCountCompleted))
    {
        reasonCode = "invalid_native_timer_create_local_state_phase";
        return false;
    }

    BackendAgentNativeTimerCreatePayload payload;
    if (!backendAgentNativeTimerCreateParsePayload(fields[1], payload, reasonCode))
    {
        reasonCode = "invalid_native_timer_create_local_state_payload";
        return false;
    }
    bool envelopeOk = false;
    candidate.command = commandFromEnvelope(fields, payload, envelopeOk);
    std::uint64_t starting = 0;
    if (!envelopeOk || !parseUnsigned(fields[12], starting) ||
        starting > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
    {
        reasonCode = "invalid_native_timer_create_local_state_envelope";
        return false;
    }
    candidate.localStartingPersistedAt = static_cast<std::int64_t>(starting);

    if (candidate.phase == BackendAgentNativeTimerCreateLocalPhase::completed)
    {
        BackendAgentNativeTimerCreateOutcomeCategory outcome;
        std::int64_t dispatchStartedAt = 0;
        std::int64_t completedAt = 0;
        if (!parseOutcome(fields[13], outcome) ||
            !parseTime(fields[14], dispatchStartedAt, false) ||
            !parseTime(fields[15], completedAt) ||
            !backendAgentCommandSafeText(fields[16], 1024))
        {
            reasonCode = "invalid_native_timer_create_local_state_evidence";
            return false;
        }
        candidate.evidence = evidenceFor(
            candidate.command,
            candidate.localStartingPersistedAt,
            outcome,
            dispatchStartedAt,
            completedAt,
            fields[16]);
    }

    if (!backendAgentNativeTimerCreateLocalStateValid(candidate, reasonCode) ||
        backendAgentNativeTimerCreateSerializeLocalState(candidate, reasonCode) != encoded)
    {
        reasonCode = "invalid_native_timer_create_local_state";
        return false;
    }
    state = std::move(candidate);
    reasonCode.clear();
    return true;
}

} // namespace vdrsuite::agent

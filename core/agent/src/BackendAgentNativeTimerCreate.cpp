#include "BackendAgentNativeTimerCreate.h"

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerCreatePayload.h"

#include <cctype>
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
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxTextLength = 1024;
constexpr std::size_t kMaxResultEvidenceLength = 4096;
constexpr const char* kResultEvidencePrefix = "native-timer-create-result-evidence/1|";

bool boundedText(const std::string& value, std::size_t maximum, bool allowEmpty = true)
{
    return (allowEmpty || !value.empty()) &&
        backendAgentCommandSafeText(value, maximum);
}

bool validHhmm(const std::string& value)
{
    if (value.empty() || value.size() > 4) return false;
    for (unsigned char character : value)
        if (std::isdigit(character) == 0) return false;
    const std::string normalized = std::string(4 - value.size(), '0') + value;
    const int hour = (normalized[0] - '0') * 10 + normalized[1] - '0';
    const int minute = (normalized[2] - '0') * 10 + normalized[3] - '0';
    return hour <= 23 && minute <= 59;
}

std::string normalizedHhmm(const std::string& value)
{
    return std::string(4 - value.size(), '0') + value;
}

void append(std::string& output, const std::string& value)
{
    output += std::to_string(value.size());
    output += ':';
    output += value;
    output += '|';
}

void append(std::string& output, std::int32_t value)
{
    append(output, std::to_string(value));
}

void append(std::string& output, bool value)
{
    append(output, std::string(value ? "1" : "0"));
}

bool exactProviderSelection(const BackendAgentLocalProviderSelection& selection)
{
    return backendAgentLocalProviderValidSelection(selection) &&
        selection.authorityDomain == kBackendAgentNativeTimerCreateAuthorityDomain &&
        selection.providerId == kBackendAgentNativeTimerCreateProviderId &&
        selection.providerKind == kBackendAgentNativeTimerCreateProviderKind &&
        selection.requiredCapability == kBackendAgentNativeTimerCreateCapability;
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
    if (!digitSeen || position >= input.size() || input[position] != ':')
        return false;
    ++position;
    if (length > input.size() - position) return false;
    value = input.substr(position, length);
    position += length;
    if (position >= input.size() || input[position] != '|') return false;
    ++position;
    return true;
}

bool parseTime(const std::string& token, std::int64_t& value, bool positive)
{
    if (token.empty()) return false;
    std::uint64_t parsed = 0;
    for (char character : token)
    {
        if (character < '0' || character > '9') return false;
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (parsed > (static_cast<std::uint64_t>(
                          std::numeric_limits<std::int64_t>::max()) - digit) / 10)
            return false;
        parsed = parsed * 10 + digit;
    }
    if (positive && parsed == 0) return false;
    value = static_cast<std::int64_t>(parsed);
    return true;
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

bool parseOutcome(const std::string& value, BackendAgentNativeTimerCreateOutcomeCategory& outcome)
{
    if (value == "rejected_without_effect")
        outcome = BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect;
    else if (value == "accepted_unverified")
        outcome = BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified;
    else if (value == "outcome_unknown")
        outcome = BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown;
    else
        return false;
    return true;
}

void populateEvidenceIdentity(
    const BackendAgentNativeTimerCreateCommand& command,
    BackendAgentNativeTimerCreateEvidence& evidence)
{
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
    evidence.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
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

bool backendAgentNativeTimerCreateSpecificationValid(
    const BackendAgentNativeTimerCreateSpecification& specification)
{
    if (!boundedText(specification.channelId, kMaxIdentityLength, false) ||
        !boundedText(specification.title, kMaxTextLength) ||
        !boundedText(specification.directory, kMaxTextLength) ||
        !boundedText(specification.day, kMaxIdentityLength) ||
        specification.weekdays.size() != 7 ||
        !validHhmm(specification.startTime) ||
        !validHhmm(specification.endTime) ||
        specification.priority < 0 || specification.priority > 99 ||
        specification.lifetime < 0 || specification.lifetime > 99)
        return false;

    for (unsigned char character : specification.weekdays)
        if (character != '-' && std::isalpha(character) == 0) return false;
    return true;
}

std::string backendAgentNativeTimerCreateSpecificationFingerprint(
    const BackendAgentNativeTimerCreateSpecification& specification)
{
    if (!backendAgentNativeTimerCreateSpecificationValid(specification)) return {};
    std::string fingerprint = "native-timer-specification/1|";
    append(fingerprint, specification.channelId);
    append(fingerprint, specification.title);
    append(fingerprint, specification.directory);
    append(fingerprint, specification.day);
    append(fingerprint, specification.weekdays);
    append(fingerprint, normalizedHhmm(specification.startTime));
    append(fingerprint, normalizedHhmm(specification.endTime));
    append(fingerprint, specification.priority);
    append(fingerprint, specification.lifetime);
    append(fingerprint, specification.enabled);
    append(fingerprint, specification.vps);
    return fingerprint;
}

bool backendAgentNativeTimerCreateValidCommand(
    const BackendAgentNativeTimerCreateCommand& command,
    std::string& reasonCode)
{
    const std::string fingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(command.specification);
    if (!backendAgentCommandSafeIdentifier(command.commandId) ||
        !backendAgentCommandSafeIdentifier(command.requestFingerprint) ||
        !backendAgentCommandSafeIdentifier(command.operationId) ||
        !backendAgentCommandSafeIdentifier(command.operationRevision) ||
        !backendAgentCommandSafeIdentifier(command.timerAssignmentId) ||
        !backendAgentCommandSafeIdentifier(command.expectedAssignmentRevision) ||
        !backendAgentCommandSafeIdentifier(command.expectedIntentRevision) ||
        command.assignmentEpoch == 0 ||
        !backendAgentCommandSafeIdentifier(command.nativeTimerBindingId) ||
        fingerprint.empty() || command.expectedSpecificationFingerprint != fingerprint ||
        !backendAgentCommandSafeIdentifier(command.jobId) ||
        !backendAgentCommandSafeIdentifier(command.attemptId) ||
        command.claimEpoch == 0 ||
        !backendAgentCommandSafeIdentifier(command.backendId) ||
        !backendAgentCommandSafeIdentifier(command.agentId) ||
        !backendAgentCommandSafeIdentifier(command.agentInstanceId) ||
        command.backendGeneration == 0 || command.controlPlaneClaimedAt <= 0 ||
        !exactProviderSelection(command.localProviderSelection) ||
        command.localProviderSelection.backendId != command.backendId)
    {
        reasonCode = "invalid_native_timer_create_command";
        return false;
    }
    reasonCode.clear();
    return true;
}

bool backendAgentNativeTimerCreateEvidenceMatches(
    const BackendAgentNativeTimerCreateEvidence& evidence,
    const BackendAgentNativeTimerCreateCommand& command,
    std::string& reasonCode)
{
    std::string commandReason;
    if (!backendAgentNativeTimerCreateValidCommand(command, commandReason) ||
        evidence.commandId != command.commandId ||
        evidence.requestFingerprint != command.requestFingerprint ||
        evidence.operationId != command.operationId ||
        evidence.operationRevision != command.operationRevision ||
        evidence.timerAssignmentId != command.timerAssignmentId ||
        evidence.nativeTimerBindingId != command.nativeTimerBindingId ||
        evidence.jobId != command.jobId || evidence.attemptId != command.attemptId ||
        evidence.claimEpoch != command.claimEpoch ||
        evidence.backendId != command.backendId || evidence.agentId != command.agentId ||
        evidence.agentInstanceId != command.agentInstanceId ||
        evidence.backendGeneration != command.backendGeneration ||
        evidence.providerInstanceEpoch != command.localProviderSelection.providerInstanceEpoch ||
        evidence.localStartingPersistedAt <= 0 || evidence.completedAt <= 0 ||
        evidence.completedAt < evidence.localStartingPersistedAt ||
        (evidence.dispatchStartedAt > 0 &&
         (evidence.dispatchStartedAt < evidence.localStartingPersistedAt ||
          evidence.completedAt < evidence.dispatchStartedAt)) ||
        (evidence.outcome == BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect &&
         evidence.dispatchStartedAt != 0) ||
        (evidence.outcome != BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect &&
         evidence.dispatchStartedAt <= 0) ||
        !backendAgentCommandSafeText(evidence.evidenceReference, 1024))
    {
        reasonCode = "native_timer_create_evidence_mismatch";
        return false;
    }
    reasonCode.clear();
    return true;
}


std::string backendAgentNativeTimerCreateResultEvidence(
    const BackendAgentNativeTimerCreateEvidence& evidence,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode)
{
    BackendAgentNativeTimerCreateCommand command;
    if (!backendAgentNativeTimerCreateCommandFromAssignment(
            assignment, command, reasonCode) ||
        !backendAgentNativeTimerCreateEvidenceMatches(
            evidence, command, reasonCode))
    {
        reasonCode = "invalid_native_timer_create_result_evidence";
        return {};
    }

    const std::string outcome = outcomeName(evidence.outcome);
    if (outcome.empty())
    {
        reasonCode = "invalid_native_timer_create_result_evidence";
        return {};
    }

    std::string encoded(kResultEvidencePrefix);
    append(encoded, outcome);
    append(encoded, std::to_string(evidence.localStartingPersistedAt));
    append(encoded, std::to_string(evidence.dispatchStartedAt));
    append(encoded, std::to_string(evidence.completedAt));
    append(encoded, evidence.evidenceReference);
    if (encoded.size() > kMaxResultEvidenceLength ||
        !backendAgentCommandSafeText(encoded, kMaxResultEvidenceLength))
    {
        reasonCode = "native_timer_create_result_evidence_too_large";
        return {};
    }
    reasonCode.clear();
    return encoded;
}

bool backendAgentNativeTimerCreateParseResultEvidence(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode)
{
    const std::string prefix(kResultEvidencePrefix);
    if (encoded.compare(0, prefix.size(), prefix) != 0 ||
        encoded.size() > kMaxResultEvidenceLength ||
        !backendAgentCommandSafeText(encoded, kMaxResultEvidenceLength))
    {
        reasonCode = "invalid_native_timer_create_result_evidence";
        return false;
    }

    std::size_t position = prefix.size();
    std::vector<std::string> fields;
    fields.reserve(5);
    while (position < encoded.size())
    {
        std::string field;
        if (!readField(encoded, position, field) || fields.size() >= 5)
        {
            reasonCode = "invalid_native_timer_create_result_evidence";
            return false;
        }
        fields.push_back(std::move(field));
    }
    if (fields.size() != 5)
    {
        reasonCode = "invalid_native_timer_create_result_evidence";
        return false;
    }

    BackendAgentNativeTimerCreateCommand command;
    BackendAgentNativeTimerCreateEvidence candidate;
    if (!backendAgentNativeTimerCreateCommandFromAssignment(
            assignment, command, reasonCode) ||
        !parseOutcome(fields[0], candidate.outcome) ||
        !parseTime(fields[1], candidate.localStartingPersistedAt, true) ||
        !parseTime(fields[2], candidate.dispatchStartedAt, false) ||
        !parseTime(fields[3], candidate.completedAt, true))
    {
        reasonCode = "invalid_native_timer_create_result_evidence";
        return false;
    }
    populateEvidenceIdentity(command, candidate);
    candidate.evidenceReference = fields[4];

    std::string canonicalReason;
    const std::string canonical = backendAgentNativeTimerCreateResultEvidence(
        candidate, assignment, canonicalReason);
    if (canonical.empty() || canonical != encoded)
    {
        reasonCode = "invalid_native_timer_create_result_evidence";
        return false;
    }

    evidence = std::move(candidate);
    reasonCode.clear();
    return true;
}

} // namespace vdrsuite::agent

#include "BackendAgentNativeTimerCreate.h"

#include "BackendAgentCommand.h"

#include <cctype>
#include <cstddef>
#include <string>

namespace vdrsuite::agent
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxTextLength = 1024;

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

} // namespace vdrsuite::agent

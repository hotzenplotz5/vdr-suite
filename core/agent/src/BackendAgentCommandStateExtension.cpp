#include "BackendAgentCommandStateExtension.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace vdrsuite::agent
{
namespace
{

constexpr std::size_t kMaximumExtensionPayloadBytes = 16U * 1024U;
constexpr std::size_t kMaximumEncodedExtensionBytes = 40U * 1024U;

bool safePayload(const std::string& value)
{
    return !value.empty() && value.size() <= kMaximumExtensionPayloadBytes &&
        std::none_of(value.begin(), value.end(), [](unsigned char character) {
            return character == 0 || character == '\r';
        });
}

char hexDigit(unsigned value)
{
    return value < 10U
        ? static_cast<char>('0' + value)
        : static_cast<char>('a' + (value - 10U));
}

std::string hexEncode(const std::string& value)
{
    std::string encoded;
    encoded.reserve(value.size() * 2U);
    for (unsigned char character : value)
    {
        encoded.push_back(hexDigit((character >> 4U) & 0x0fU));
        encoded.push_back(hexDigit(character & 0x0fU));
    }
    return encoded;
}

int hexValue(unsigned char character)
{
    if (character >= '0' && character <= '9')
        return static_cast<int>(character - '0');
    if (character >= 'a' && character <= 'f')
        return static_cast<int>(character - 'a' + 10U);
    return -1;
}

bool hexDecode(const std::string& encoded, std::string& value)
{
    value.clear();
    if ((encoded.size() % 2U) != 0 || encoded.size() > kMaximumEncodedExtensionBytes)
        return false;
    value.reserve(encoded.size() / 2U);
    for (std::size_t offset = 0; offset < encoded.size(); offset += 2U)
    {
        const int high = hexValue(static_cast<unsigned char>(encoded[offset]));
        const int low = hexValue(static_cast<unsigned char>(encoded[offset + 1U]));
        if (high < 0 || low < 0) return false;
        value.push_back(static_cast<char>((high << 4) | low));
    }
    return true;
}

bool splitEncoded(
    const std::string& encoded,
    std::vector<std::string>& fields)
{
    fields.clear();
    if (encoded.empty() || encoded.size() > kMaximumEncodedExtensionBytes)
        return false;
    std::size_t start = 0;
    while (start <= encoded.size())
    {
        const std::size_t separator = encoded.find('.', start);
        fields.push_back(encoded.substr(
            start,
            separator == std::string::npos
                ? std::string::npos
                : separator - start));
        if (separator == std::string::npos) break;
        start = separator + 1U;
    }
    return fields.size() == 5U && fields.front() == "cse1";
}

bool sameProviderSelection(
    const BackendAgentLocalProviderSelection& left,
    const BackendAgentLocalProviderSelection& right)
{
    return left.backendId == right.backendId &&
        left.authorityDomain == right.authorityDomain &&
        left.providerId == right.providerId &&
        left.providerKind == right.providerKind &&
        left.ownershipGeneration == right.ownershipGeneration &&
        left.providerInstanceEpoch == right.providerInstanceEpoch &&
        left.providerGeneration == right.providerGeneration &&
        left.capabilityRevision == right.capabilityRevision &&
        left.requiredCapability == right.requiredCapability;
}

bool sameCreateSpecification(
    const BackendAgentNativeTimerCreateSpecification& left,
    const BackendAgentNativeTimerCreateSpecification& right)
{
    return left.channelId == right.channelId &&
        left.title == right.title &&
        left.directory == right.directory &&
        left.day == right.day &&
        left.weekdays == right.weekdays &&
        left.startTime == right.startTime &&
        left.endTime == right.endTime &&
        left.priority == right.priority &&
        left.lifetime == right.lifetime &&
        left.enabled == right.enabled &&
        left.vps == right.vps;
}

bool sameCommand(
    const BackendAgentNativeTimerCreateCommand& left,
    const BackendAgentNativeTimerCreateCommand& right)
{
    return left.commandId == right.commandId &&
        left.requestFingerprint == right.requestFingerprint &&
        left.operationId == right.operationId &&
        left.operationRevision == right.operationRevision &&
        left.timerAssignmentId == right.timerAssignmentId &&
        left.expectedAssignmentRevision == right.expectedAssignmentRevision &&
        left.expectedIntentRevision == right.expectedIntentRevision &&
        left.assignmentEpoch == right.assignmentEpoch &&
        left.nativeTimerBindingId == right.nativeTimerBindingId &&
        left.expectedSpecificationFingerprint == right.expectedSpecificationFingerprint &&
        left.jobId == right.jobId &&
        left.attemptId == right.attemptId &&
        left.claimEpoch == right.claimEpoch &&
        left.backendId == right.backendId &&
        left.agentId == right.agentId &&
        left.agentInstanceId == right.agentInstanceId &&
        left.backendGeneration == right.backendGeneration &&
        left.controlPlaneClaimedAt == right.controlPlaneClaimedAt &&
        sameCreateSpecification(left.specification, right.specification) &&
        sameProviderSelection(left.localProviderSelection, right.localProviderSelection);
}

bool sameCommand(
    const BackendAgentNativeTimerDeleteCommand& left,
    const BackendAgentNativeTimerDeleteCommand& right)
{
    return left.commandId == right.commandId &&
        left.requestFingerprint == right.requestFingerprint &&
        left.operationId == right.operationId &&
        left.operationRevision == right.operationRevision &&
        left.nativeTimerBindingId == right.nativeTimerBindingId &&
        left.expectedBindingRevision == right.expectedBindingRevision &&
        left.timerAssignmentId == right.timerAssignmentId &&
        left.backendNativeTimerId == right.backendNativeTimerId &&
        left.jobId == right.jobId &&
        left.attemptId == right.attemptId &&
        left.claimEpoch == right.claimEpoch &&
        left.backendId == right.backendId &&
        left.agentId == right.agentId &&
        left.agentInstanceId == right.agentInstanceId &&
        left.backendGeneration == right.backendGeneration &&
        left.controlPlaneClaimedAt == right.controlPlaneClaimedAt &&
        sameProviderSelection(left.localProviderSelection, right.localProviderSelection);
}


bool sameCommand(
    const BackendAgentNativeTimerModifyCommand& left,
    const BackendAgentNativeTimerModifyCommand& right)
{
    return left.kind == right.kind &&
        left.commandId == right.commandId &&
        left.requestFingerprint == right.requestFingerprint &&
        left.operationId == right.operationId &&
        left.operationRevision == right.operationRevision &&
        left.timerAssignmentId == right.timerAssignmentId &&
        left.expectedAssignmentRevision == right.expectedAssignmentRevision &&
        left.expectedIntentRevision == right.expectedIntentRevision &&
        left.assignmentEpoch == right.assignmentEpoch &&
        left.nativeTimerBindingId == right.nativeTimerBindingId &&
        left.expectedBindingRevision == right.expectedBindingRevision &&
        left.backendNativeTimerId == right.backendNativeTimerId &&
        left.expectedCurrentFingerprint == right.expectedCurrentFingerprint &&
        left.expectedSpecificationFingerprint == right.expectedSpecificationFingerprint &&
        sameCreateSpecification(left.specification, right.specification) &&
        left.jobId == right.jobId && left.attemptId == right.attemptId &&
        left.claimEpoch == right.claimEpoch && left.backendId == right.backendId &&
        left.agentId == right.agentId &&
        left.agentInstanceId == right.agentInstanceId &&
        left.backendGeneration == right.backendGeneration &&
        left.controlPlaneClaimedAt == right.controlPlaneClaimedAt &&
        sameProviderSelection(left.localProviderSelection, right.localProviderSelection);
}

} // namespace

bool backendAgentCommandStateExtensionValid(
    const BackendAgentCommandStateExtension& extension,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode)
{
    if (extension.schemaVersion != 1 ||
        !::backendAgentCommandValidAssignment(assignment) ||
        !::backendAgentCommandSafeIdentifier(extension.extensionType) ||
        extension.commandId != assignment.commandId ||
        extension.requestFingerprint != assignment.requestFingerprint ||
        !safePayload(extension.payload))
    {
        reasonCode = "invalid_command_state_extension";
        return false;
    }
    reasonCode.clear();
    return true;
}

std::string backendAgentCommandStateExtensionSerialize(
    const BackendAgentCommandStateExtension& extension,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode)
{
    if (!backendAgentCommandStateExtensionValid(extension, assignment, reasonCode))
        return {};

    std::string encoded = "cse1." +
        hexEncode(extension.extensionType) + "." +
        hexEncode(extension.commandId) + "." +
        hexEncode(extension.requestFingerprint) + "." +
        hexEncode(extension.payload);
    if (encoded.size() > kMaximumEncodedExtensionBytes)
    {
        reasonCode = "command_state_extension_too_large";
        return {};
    }
    reasonCode.clear();
    return encoded;
}

bool backendAgentCommandStateExtensionParse(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentCommandStateExtension& extension,
    std::string& reasonCode)
{
    std::vector<std::string> fields;
    if (!splitEncoded(encoded, fields))
    {
        reasonCode = "invalid_command_state_extension_encoding";
        return false;
    }

    BackendAgentCommandStateExtension candidate;
    candidate.schemaVersion = 1;
    if (!hexDecode(fields[1], candidate.extensionType) ||
        !hexDecode(fields[2], candidate.commandId) ||
        !hexDecode(fields[3], candidate.requestFingerprint) ||
        !hexDecode(fields[4], candidate.payload) ||
        !backendAgentCommandStateExtensionValid(candidate, assignment, reasonCode))
    {
        reasonCode = "invalid_command_state_extension";
        return false;
    }

    extension = candidate;
    reasonCode.clear();
    return true;
}

bool backendAgentCommandStateExtensionValidateSupported(
    const BackendAgentCommandStateExtension& extension,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode)
{
    if (!backendAgentCommandStateExtensionValid(
            extension, assignment, reasonCode))
        return false;

    if (extension.extensionType ==
        kBackendAgentNativeTimerCreateLocalStateExtensionType)
    {
        BackendAgentNativeTimerCreateLocalState candidate;
        BackendAgentNativeTimerCreateCommand expected;
        if (!backendAgentNativeTimerCreateParseLocalState(
                extension.payload, candidate, reasonCode) ||
            !backendAgentNativeTimerCreateCommandFromAssignment(
                assignment, expected, reasonCode) ||
            !sameCommand(expected, candidate.command))
        {
            reasonCode =
                "native_timer_create_state_extension_assignment_mismatch";
            return false;
        }
        reasonCode.clear();
        return true;
    }

    if (extension.extensionType ==
        kBackendAgentNativeTimerDeleteLocalStateExtensionType)
    {
        BackendAgentNativeTimerDeleteLocalState candidate;
        BackendAgentNativeTimerDeleteCommand expected;
        if (!backendAgentNativeTimerDeleteParseLocalState(
                extension.payload, candidate, reasonCode) ||
            !backendAgentNativeTimerDeleteCommandFromAssignment(
                assignment, expected, reasonCode) ||
            !sameCommand(expected, candidate.command))
        {
            reasonCode =
                "native_timer_delete_state_extension_assignment_mismatch";
            return false;
        }
        reasonCode.clear();
        return true;
    }


    if (extension.extensionType ==
        kBackendAgentNativeTimerModifyLocalStateExtensionType)
    {
        BackendAgentNativeTimerModifyLocalState candidate;
        BackendAgentNativeTimerModifyCommand expected;
        if (!backendAgentNativeTimerModifyParseLocalState(
                extension.payload, candidate, reasonCode) ||
            !backendAgentNativeTimerModifyCommandFromAssignment(
                assignment, expected, reasonCode) ||
            !sameCommand(expected, candidate.command))
        {
            reasonCode =
                "native_timer_modify_state_extension_assignment_mismatch";
            return false;
        }
        reasonCode.clear();
        return true;
    }

    reasonCode = "unsupported_command_state_extension_type";
    return false;
}

std::string backendAgentNativeTimerCreateCommandStateExtension(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode)
{
    BackendAgentNativeTimerCreateCommand expected;
    if (!backendAgentNativeTimerCreateCommandFromAssignment(
            assignment, expected, reasonCode) ||
        !backendAgentNativeTimerCreateLocalStateValid(state, reasonCode) ||
        !sameCommand(expected, state.command))
    {
        reasonCode = "native_timer_create_state_extension_assignment_mismatch";
        return {};
    }

    const std::string payload =
        backendAgentNativeTimerCreateSerializeLocalState(state, reasonCode);
    if (payload.empty()) return {};

    BackendAgentCommandStateExtension extension;
    extension.extensionType = kBackendAgentNativeTimerCreateLocalStateExtensionType;
    extension.commandId = assignment.commandId;
    extension.requestFingerprint = assignment.requestFingerprint;
    extension.payload = payload;
    return backendAgentCommandStateExtensionSerialize(
        extension, assignment, reasonCode);
}

bool backendAgentNativeTimerCreateParseCommandStateExtension(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode)
{
    BackendAgentCommandStateExtension extension;
    if (!backendAgentCommandStateExtensionParse(
            encoded, assignment, extension, reasonCode) ||
        extension.extensionType !=
            kBackendAgentNativeTimerCreateLocalStateExtensionType)
    {
        reasonCode = "native_timer_create_state_extension_type_mismatch";
        return false;
    }

    BackendAgentNativeTimerCreateLocalState candidate;
    BackendAgentNativeTimerCreateCommand expected;
    if (!backendAgentNativeTimerCreateParseLocalState(
            extension.payload, candidate, reasonCode) ||
        !backendAgentNativeTimerCreateCommandFromAssignment(
            assignment, expected, reasonCode) ||
        !sameCommand(expected, candidate.command))
    {
        reasonCode = "native_timer_create_state_extension_assignment_mismatch";
        return false;
    }

    state = candidate;
    reasonCode.clear();
    return true;
}

std::string backendAgentNativeTimerDeleteCommandStateExtension(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode)
{
    BackendAgentNativeTimerDeleteCommand expected;
    if (!backendAgentNativeTimerDeleteCommandFromAssignment(
            assignment, expected, reasonCode) ||
        !backendAgentNativeTimerDeleteLocalStateValid(state, reasonCode) ||
        !sameCommand(expected, state.command))
    {
        reasonCode = "native_timer_delete_state_extension_assignment_mismatch";
        return {};
    }

    const std::string payload =
        backendAgentNativeTimerDeleteSerializeLocalState(state, reasonCode);
    if (payload.empty()) return {};

    BackendAgentCommandStateExtension extension;
    extension.extensionType = kBackendAgentNativeTimerDeleteLocalStateExtensionType;
    extension.commandId = assignment.commandId;
    extension.requestFingerprint = assignment.requestFingerprint;
    extension.payload = payload;
    return backendAgentCommandStateExtensionSerialize(
        extension, assignment, reasonCode);
}

bool backendAgentNativeTimerDeleteParseCommandStateExtension(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode)
{
    BackendAgentCommandStateExtension extension;
    if (!backendAgentCommandStateExtensionParse(
            encoded, assignment, extension, reasonCode) ||
        extension.extensionType !=
            kBackendAgentNativeTimerDeleteLocalStateExtensionType)
    {
        reasonCode = "native_timer_delete_state_extension_type_mismatch";
        return false;
    }

    BackendAgentNativeTimerDeleteLocalState candidate;
    BackendAgentNativeTimerDeleteCommand expected;
    if (!backendAgentNativeTimerDeleteParseLocalState(
            extension.payload, candidate, reasonCode) ||
        !backendAgentNativeTimerDeleteCommandFromAssignment(
            assignment, expected, reasonCode) ||
        !sameCommand(expected, candidate.command))
    {
        reasonCode = "native_timer_delete_state_extension_assignment_mismatch";
        return false;
    }

    state = candidate;
    reasonCode.clear();
    return true;
}


std::string backendAgentNativeTimerModifyCommandStateExtension(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerModifyLocalState& state,
    std::string& reasonCode)
{
    BackendAgentNativeTimerModifyCommand expected;
    if (!backendAgentNativeTimerModifyCommandFromAssignment(
            assignment, expected, reasonCode) ||
        !backendAgentNativeTimerModifyLocalStateValid(state, reasonCode) ||
        !sameCommand(expected, state.command))
    {
        reasonCode = "native_timer_modify_state_extension_assignment_mismatch";
        return {};
    }
    const std::string payload =
        backendAgentNativeTimerModifySerializeLocalState(state, reasonCode);
    if (payload.empty()) return {};
    BackendAgentCommandStateExtension extension;
    extension.extensionType = kBackendAgentNativeTimerModifyLocalStateExtensionType;
    extension.commandId = assignment.commandId;
    extension.requestFingerprint = assignment.requestFingerprint;
    extension.payload = payload;
    return backendAgentCommandStateExtensionSerialize(
        extension, assignment, reasonCode);
}

bool backendAgentNativeTimerModifyParseCommandStateExtension(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerModifyLocalState& state,
    std::string& reasonCode)
{
    BackendAgentCommandStateExtension extension;
    if (!backendAgentCommandStateExtensionParse(
            encoded, assignment, extension, reasonCode) ||
        extension.extensionType !=
            kBackendAgentNativeTimerModifyLocalStateExtensionType)
    {
        reasonCode = "native_timer_modify_state_extension_type_mismatch";
        return false;
    }
    BackendAgentNativeTimerModifyLocalState candidate;
    BackendAgentNativeTimerModifyCommand expected;
    if (!backendAgentNativeTimerModifyParseLocalState(
            extension.payload, candidate, reasonCode) ||
        !backendAgentNativeTimerModifyCommandFromAssignment(
            assignment, expected, reasonCode) ||
        !sameCommand(expected, candidate.command))
    {
        reasonCode = "native_timer_modify_state_extension_assignment_mismatch";
        return false;
    }
    state = candidate;
    reasonCode.clear();
    return true;
}

} // namespace vdrsuite::agent

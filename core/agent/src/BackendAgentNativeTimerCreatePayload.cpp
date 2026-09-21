#include "BackendAgentNativeTimerCreatePayload.h"

#include "BackendAgentCommand.h"

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
constexpr const char* kPayloadPrefix = "native-timer-create-agent-payload/1|";
constexpr std::size_t kFieldCount = 28;

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
void appendSigned(std::string& output, std::int32_t value)
{
    appendField(output, std::to_string(value));
}
void appendBool(std::string& output, bool value)
{
    appendField(output, value ? "1" : "0");
}
std::string normalizedHhmm(const std::string& value)
{
    return std::string(4 - value.size(), '0') + value;
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

bool parseSmallInt(const std::string& token, std::int32_t& value)
{
    std::uint64_t parsed = 0;
    if (!parseUnsigned(token, parsed, false) || parsed > 99) return false;
    value = static_cast<std::int32_t>(parsed);
    return true;
}

bool parseBool(const std::string& token, bool& value)
{
    if (token == "0") { value = false; return true; }
    if (token == "1") { value = true; return true; }
    return false;
}
}

bool backendAgentNativeTimerCreatePayloadValid(
    const BackendAgentNativeTimerCreatePayload& payload)
{
    return backendAgentCommandSafeIdentifier(payload.operationRevision) &&
        backendAgentCommandSafeIdentifier(payload.timerAssignmentId) &&
        backendAgentCommandSafeIdentifier(payload.expectedAssignmentRevision) &&
        backendAgentCommandSafeIdentifier(payload.expectedIntentRevision) &&
        payload.assignmentEpoch > 0 &&
        backendAgentCommandSafeIdentifier(payload.nativeTimerBindingId) &&
        payload.controlPlaneClaimedAt > 0 &&
        backendAgentNativeTimerCreateSpecificationValid(payload.specification) &&
        payload.expectedSpecificationFingerprint ==
            backendAgentNativeTimerCreateSpecificationFingerprint(payload.specification) &&
        exactProviderSelection(payload.localProviderSelection);
}

std::string backendAgentNativeTimerCreatePayload(
    const BackendAgentNativeTimerCreatePayload& payload)
{
    if (!backendAgentNativeTimerCreatePayloadValid(payload)) return {};
    std::string output(kPayloadPrefix);
    appendField(output, payload.operationRevision);
    appendField(output, payload.timerAssignmentId);
    appendField(output, payload.expectedAssignmentRevision);
    appendField(output, payload.expectedIntentRevision);
    appendUnsigned(output, payload.assignmentEpoch);
    appendField(output, payload.nativeTimerBindingId);
    appendUnsigned(output, static_cast<std::uint64_t>(payload.controlPlaneClaimedAt));
    appendField(output, payload.expectedSpecificationFingerprint);

    const auto& specification = payload.specification;
    appendField(output, specification.channelId);
    appendField(output, specification.title);
    appendField(output, specification.directory);
    appendField(output, specification.day);
    appendField(output, specification.weekdays);
    appendField(output, normalizedHhmm(specification.startTime));
    appendField(output, normalizedHhmm(specification.endTime));
    appendSigned(output, specification.priority);
    appendSigned(output, specification.lifetime);
    appendBool(output, specification.enabled);
    appendBool(output, specification.vps);

    const auto& selection = payload.localProviderSelection;
    appendField(output, selection.backendId);
    appendField(output, selection.authorityDomain);
    appendField(output, selection.providerId);
    appendField(output, selection.providerKind);
    appendUnsigned(output, selection.ownershipGeneration);
    appendField(output, selection.providerInstanceEpoch);
    appendUnsigned(output, selection.providerGeneration);
    appendUnsigned(output, selection.capabilityRevision);
    appendField(output, selection.requiredCapability);
    return output;
}

bool backendAgentNativeTimerCreateParsePayload(
    const std::string& encoded,
    BackendAgentNativeTimerCreatePayload& payload,
    std::string& reasonCode)
{
    const std::string prefix(kPayloadPrefix);
    if (encoded.compare(0, prefix.size(), prefix) != 0)
    {
        reasonCode = "invalid_native_timer_create_payload";
        return false;
    }
    std::size_t position = prefix.size();
    std::vector<std::string> fields;
    fields.reserve(kFieldCount);
    while (position < encoded.size())
    {
        std::string field;
        if (!readField(encoded, position, field) || fields.size() >= kFieldCount)
        {
            reasonCode = "invalid_native_timer_create_payload";
            return false;
        }
        fields.push_back(std::move(field));
    }
    if (fields.size() != kFieldCount)
    {
        reasonCode = "invalid_native_timer_create_payload";
        return false;
    }

    BackendAgentNativeTimerCreatePayload candidate;
    candidate.operationRevision = fields[0];
    candidate.timerAssignmentId = fields[1];
    candidate.expectedAssignmentRevision = fields[2];
    candidate.expectedIntentRevision = fields[3];
    if (!parseUnsigned(fields[4], candidate.assignmentEpoch))
    { reasonCode = "invalid_native_timer_create_payload"; return false; }
    candidate.nativeTimerBindingId = fields[5];
    {
        std::uint64_t claimedAt = 0;
        if (!parseUnsigned(fields[6], claimedAt) ||
            claimedAt > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            { reasonCode = "invalid_native_timer_create_payload"; return false; }
        candidate.controlPlaneClaimedAt = static_cast<std::int64_t>(claimedAt);
    }
    candidate.expectedSpecificationFingerprint = fields[7];
    auto& specification = candidate.specification;
    specification.channelId = fields[8];
    specification.title = fields[9];
    specification.directory = fields[10];
    specification.day = fields[11];
    specification.weekdays = fields[12];
    specification.startTime = fields[13];
    specification.endTime = fields[14];
    if (!parseSmallInt(fields[15], specification.priority) ||
        !parseSmallInt(fields[16], specification.lifetime) ||
        !parseBool(fields[17], specification.enabled) ||
        !parseBool(fields[18], specification.vps))
    { reasonCode = "invalid_native_timer_create_payload"; return false; }

    auto& selection = candidate.localProviderSelection;
    selection.backendId = fields[19];
    selection.authorityDomain = fields[20];
    selection.providerId = fields[21];
    selection.providerKind = fields[22];
    if (!parseUnsigned(fields[23], selection.ownershipGeneration))
    { reasonCode = "invalid_native_timer_create_payload"; return false; }
    selection.providerInstanceEpoch = fields[24];
    if (!parseUnsigned(fields[25], selection.providerGeneration) ||
        !parseUnsigned(fields[26], selection.capabilityRevision))
    { reasonCode = "invalid_native_timer_create_payload"; return false; }
    selection.requiredCapability = fields[27];

    if (!backendAgentNativeTimerCreatePayloadValid(candidate) ||
        backendAgentNativeTimerCreatePayload(candidate) != encoded)
    { reasonCode = "invalid_native_timer_create_payload"; return false; }
    payload = std::move(candidate);
    reasonCode.clear();
    return true;
}

} // namespace vdrsuite::agent

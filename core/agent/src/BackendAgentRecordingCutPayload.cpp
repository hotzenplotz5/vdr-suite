#include "BackendAgentRecordingCutPayload.h"

#include <limits>
#include <sstream>

namespace vdrsuite::agent
{
namespace
{
constexpr const char* PayloadProtocol = "vdr-suite-recording-cut/1";
constexpr std::size_t FieldCount = 16;

void appendField(std::ostringstream& out, const std::string& value)
{
    out << value.size() << ':' << value << '|';
}

void appendField(std::ostringstream& out, std::uint64_t value)
{
    appendField(out, std::to_string(value));
}

void appendField(std::ostringstream& out, std::int64_t value)
{
    appendField(out, std::to_string(value));
}

bool readField(
    const std::string& encoded,
    std::size_t& position,
    std::string& value)
{
    const std::size_t colon = encoded.find(':', position);
    if (colon == std::string::npos || colon == position || colon - position > 10)
        return false;

    std::size_t length = 0;
    for (std::size_t index = position; index < colon; ++index)
    {
        const unsigned char character = encoded[index];
        if (character < '0' || character > '9') return false;
        const std::size_t digit = static_cast<std::size_t>(character - '0');
        if (length > (std::numeric_limits<std::size_t>::max() - digit) / 10U)
            return false;
        length = length * 10U + digit;
    }

    const std::size_t start = colon + 1;
    if (length > encoded.size() - start) return false;
    const std::size_t end = start + length;
    if (end >= encoded.size() || encoded[end] != '|') return false;
    value = encoded.substr(start, length);
    position = end + 1;
    return true;
}

bool parseUnsigned(
    const std::string& value,
    std::uint64_t& parsed,
    bool requirePositive)
{
    if (value.empty() || value.size() > 20) return false;
    parsed = 0;
    for (const unsigned char character : value)
    {
        if (character < '0' || character > '9') return false;
        const std::uint64_t digit = static_cast<std::uint64_t>(character - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
            return false;
        parsed = parsed * 10U + digit;
    }
    return !requirePositive || parsed > 0;
}

bool parsePositiveTime(const std::string& value, std::int64_t& parsed)
{
    std::uint64_t unsignedValue = 0;
    if (!parseUnsigned(value, unsignedValue, true) ||
        unsignedValue > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
    {
        return false;
    }
    parsed = static_cast<std::int64_t>(unsignedValue);
    return true;
}

bool exactSelection(const BackendAgentRecordingCutPayload& payload)
{
    const auto& selection = payload.localProviderSelection;
    return backendAgentLocalProviderValidSelection(selection) &&
        selection.backendId == payload.backendId &&
        selection.authorityDomain == kBackendAgentRecordingCutAuthorityDomain &&
        selection.providerId == kBackendAgentRecordingCutProviderId &&
        selection.providerKind == kBackendAgentRecordingCutProviderKind &&
        selection.requiredCapability == kBackendAgentRecordingCutCapability;
}

}

bool backendAgentRecordingCutValidPayload(
    const BackendAgentRecordingCutPayload& payload,
    std::string& reasonCode)
{
    if (!backendAgentCommandSafeIdentifier(payload.operationRevision) ||
        !backendAgentRecordingCutRevisionTokenValid(payload.recordingKey) ||
        !backendAgentRecordingCutRevisionTokenValid(
            payload.expectedMarksRevision) ||
        !backendAgentCommandSafeIdentifier(payload.backendId) ||
        payload.backendGeneration == 0 ||
        payload.controlPlaneClaimedAt <= 0 ||
        !exactSelection(payload))
    {
        reasonCode = "invalid_recording_cut_payload";
        return false;
    }
    reasonCode.clear();
    return true;
}

std::string backendAgentRecordingCutPayload(
    const BackendAgentRecordingCutPayload& payload)
{
    std::string reasonCode;
    if (!backendAgentRecordingCutValidPayload(payload, reasonCode)) return {};

    const auto& selection = payload.localProviderSelection;
    std::ostringstream out;
    appendField(out, PayloadProtocol);
    appendField(out, payload.operationRevision);
    appendField(out, payload.recordingKey);
    appendField(out, payload.expectedMarksRevision);
    appendField(out, payload.backendId);
    appendField(out, payload.backendGeneration);
    appendField(out, payload.controlPlaneClaimedAt);
    appendField(out, selection.backendId);
    appendField(out, selection.authorityDomain);
    appendField(out, selection.providerId);
    appendField(out, selection.providerKind);
    appendField(out, selection.ownershipGeneration);
    appendField(out, selection.providerInstanceEpoch);
    appendField(out, selection.providerGeneration);
    appendField(out, selection.capabilityRevision);
    appendField(out, selection.requiredCapability);
    return out.str();
}

bool backendAgentRecordingCutParsePayload(
    const std::string& encoded,
    BackendAgentRecordingCutPayload& payload,
    std::string& reasonCode)
{
    payload = {};
    std::string fields[FieldCount];
    std::size_t position = 0;
    for (std::size_t index = 0; index < FieldCount; ++index)
    {
        if (!readField(encoded, position, fields[index]))
        {
            reasonCode = "recording_cut_payload_malformed";
            return false;
        }
    }
    if (position != encoded.size() || fields[0] != PayloadProtocol)
    {
        reasonCode = "recording_cut_payload_malformed";
        return false;
    }

    auto& selection = payload.localProviderSelection;
    std::uint64_t backendGeneration = 0;
    std::uint64_t ownershipGeneration = 0;
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    if (!parseUnsigned(fields[5], backendGeneration, true) ||
        !parsePositiveTime(fields[6], payload.controlPlaneClaimedAt) ||
        !parseUnsigned(fields[11], ownershipGeneration, true) ||
        !parseUnsigned(fields[13], providerGeneration, true) ||
        !parseUnsigned(fields[14], capabilityRevision, true))
    {
        reasonCode = "recording_cut_payload_malformed";
        return false;
    }

    payload.operationRevision = fields[1];
    payload.recordingKey = fields[2];
    payload.expectedMarksRevision = fields[3];
    payload.backendId = fields[4];
    payload.backendGeneration = backendGeneration;
    selection.backendId = fields[7];
    selection.authorityDomain = fields[8];
    selection.providerId = fields[9];
    selection.providerKind = fields[10];
    selection.ownershipGeneration = ownershipGeneration;
    selection.providerInstanceEpoch = fields[12];
    selection.providerGeneration = providerGeneration;
    selection.capabilityRevision = capabilityRevision;
    selection.requiredCapability = fields[15];

    if (!backendAgentRecordingCutValidPayload(payload, reasonCode) ||
        backendAgentRecordingCutPayload(payload) != encoded)
    {
        if (reasonCode.empty()) reasonCode = "recording_cut_payload_not_canonical";
        return false;
    }

    reasonCode.clear();
    return true;
}

}

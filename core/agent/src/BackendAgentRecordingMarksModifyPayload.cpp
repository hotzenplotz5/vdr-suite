#include "BackendAgentRecordingMarksModifyPayload.h"

#include <limits>
#include <sstream>

namespace vdrsuite::agent
{
namespace
{
constexpr const char* PayloadProtocol =
    "vdr-suite-recording-marks-modify/1";
constexpr std::size_t FieldCount = 20;

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

void appendFrame(std::ostringstream& out, int frame)
{
    appendField(out, frame < 0 ? std::string("-") : std::to_string(frame));
}

std::string replacementFramesToken(const std::vector<int>& frames)
{
    if (frames.empty()) return "-";
    std::ostringstream out;
    for (std::size_t index = 0; index < frames.size(); ++index)
    {
        if (index != 0) out << ',';
        out << frames[index];
    }
    return out.str();
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

bool parseFrame(const std::string& value, int& frame)
{
    if (value == "-")
    {
        frame = -1;
        return true;
    }
    std::uint64_t parsed = 0;
    if (!parseUnsigned(value, parsed, false) ||
        parsed > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
    {
        return false;
    }
    frame = static_cast<int>(parsed);
    return true;
}

bool parseReplacementFrames(
    const std::string& value,
    std::vector<int>& frames)
{
    frames.clear();
    if (value == "-") return true;
    if (value.empty()) return false;

    std::size_t position = 0;
    while (position < value.size())
    {
        const std::size_t end = value.find(',', position);
        const std::string token = value.substr(
            position,
            end == std::string::npos ? std::string::npos : end - position);
        int frame = -1;
        if (!parseFrame(token, frame) || frame < 0) return false;
        frames.push_back(frame);
        if (frames.size() > kBackendAgentRecordingMarksMaximumReplacementFrames)
            return false;
        if (end == std::string::npos) break;
        position = end + 1;
        if (position == value.size()) return false;
    }
    return true;
}

bool parseKind(
    const std::string& value,
    BackendAgentRecordingMarksModifyKind& kind)
{
    if (value == "add") kind = BackendAgentRecordingMarksModifyKind::add;
    else if (value == "delete")
        kind = BackendAgentRecordingMarksModifyKind::deleteMark;
    else if (value == "move") kind = BackendAgentRecordingMarksModifyKind::move;
    else if (value == "reset") kind = BackendAgentRecordingMarksModifyKind::reset;
    else if (value == "replace") kind = BackendAgentRecordingMarksModifyKind::replace;
    else return false;
    return true;
}

bool exactSelection(const BackendAgentRecordingMarksModifyPayload& payload)
{
    const auto& selection = payload.localProviderSelection;
    return backendAgentLocalProviderValidSelection(selection) &&
        selection.backendId == payload.backendId &&
        selection.authorityDomain ==
            kBackendAgentRecordingMarksModifyAuthorityDomain &&
        selection.providerId == kBackendAgentRecordingMarksModifyProviderId &&
        selection.providerKind == kBackendAgentRecordingMarksModifyProviderKind &&
        selection.requiredCapability ==
            kBackendAgentRecordingMarksModifyCapability;
}

}

bool backendAgentRecordingMarksModifyValidPayload(
    const BackendAgentRecordingMarksModifyPayload& payload,
    std::string& reasonCode)
{
    if (std::string(backendAgentRecordingMarksModifyKindName(payload.kind)) ==
            "invalid" ||
        !backendAgentCommandSafeIdentifier(payload.operationRevision) ||
        !backendAgentRecordingMarksModifyRevisionTokenValid(payload.recordingKey) ||
        !backendAgentRecordingMarksModifyRevisionTokenValid(
            payload.expectedMarksRevision) ||
        !backendAgentRecordingMarksModifyFrameShapeValid(
            payload.kind,
            payload.sourceFrame,
            payload.targetFrame,
            payload.replacementFrames) ||
        !backendAgentCommandSafeIdentifier(payload.backendId) ||
        payload.backendGeneration == 0 ||
        payload.controlPlaneClaimedAt <= 0 ||
        !exactSelection(payload))
    {
        reasonCode = "invalid_recording_marks_modify_payload";
        return false;
    }
    reasonCode.clear();
    return true;
}

std::string backendAgentRecordingMarksModifyPayload(
    const BackendAgentRecordingMarksModifyPayload& payload)
{
    std::string reasonCode;
    if (!backendAgentRecordingMarksModifyValidPayload(payload, reasonCode))
        return {};

    const auto& selection = payload.localProviderSelection;
    std::ostringstream out;
    appendField(out, PayloadProtocol);
    appendField(out, backendAgentRecordingMarksModifyKindName(payload.kind));
    appendField(out, payload.operationRevision);
    appendField(out, payload.recordingKey);
    appendField(out, payload.expectedMarksRevision);
    appendFrame(out, payload.sourceFrame);
    appendFrame(out, payload.targetFrame);
    appendField(out, replacementFramesToken(payload.replacementFrames));
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

bool backendAgentRecordingMarksModifyParsePayload(
    const std::string& encoded,
    BackendAgentRecordingMarksModifyPayload& payload,
    std::string& reasonCode)
{
    payload = {};
    std::string fields[FieldCount];
    std::size_t position = 0;
    for (std::size_t index = 0; index < FieldCount; ++index)
    {
        if (!readField(encoded, position, fields[index]))
        {
            reasonCode = "recording_marks_modify_payload_malformed";
            return false;
        }
    }
    if (position != encoded.size() || fields[0] != PayloadProtocol)
    {
        reasonCode = "recording_marks_modify_payload_malformed";
        return false;
    }

    auto& selection = payload.localProviderSelection;
    std::uint64_t backendGeneration = 0;
    std::uint64_t ownershipGeneration = 0;
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    if (!parseKind(fields[1], payload.kind) ||
        !parseFrame(fields[5], payload.sourceFrame) ||
        !parseFrame(fields[6], payload.targetFrame) ||
        !parseReplacementFrames(fields[7], payload.replacementFrames) ||
        !parseUnsigned(fields[9], backendGeneration, true) ||
        !parsePositiveTime(fields[10], payload.controlPlaneClaimedAt) ||
        !parseUnsigned(fields[15], ownershipGeneration, true) ||
        !parseUnsigned(fields[17], providerGeneration, true) ||
        !parseUnsigned(fields[18], capabilityRevision, true))
    {
        reasonCode = "recording_marks_modify_payload_malformed";
        return false;
    }

    payload.operationRevision = fields[2];
    payload.recordingKey = fields[3];
    payload.expectedMarksRevision = fields[4];
    payload.backendId = fields[8];
    payload.backendGeneration = backendGeneration;
    selection.backendId = fields[11];
    selection.authorityDomain = fields[12];
    selection.providerId = fields[13];
    selection.providerKind = fields[14];
    selection.ownershipGeneration = ownershipGeneration;
    selection.providerInstanceEpoch = fields[16];
    selection.providerGeneration = providerGeneration;
    selection.capabilityRevision = capabilityRevision;
    selection.requiredCapability = fields[19];

    if (!backendAgentRecordingMarksModifyValidPayload(payload, reasonCode) ||
        backendAgentRecordingMarksModifyPayload(payload) != encoded)
    {
        if (reasonCode.empty())
            reasonCode = "recording_marks_modify_payload_not_canonical";
        return false;
    }

    reasonCode.clear();
    return true;
}

}

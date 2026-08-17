#include "NativeTimerModifyOperationPayload.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t MaxIdentity = 160;
constexpr std::size_t MaxFingerprint = 4096;
constexpr const char* PayloadPrefix =
    "native-timer-modify-operation-payload/1|";
constexpr const char* FingerprintPrefix =
    "native-timer-modify-operation-payload-fingerprint/1|";

bool identity(const std::string& value)
{
    return !value.empty() && value.size() <= MaxIdentity;
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

void append(std::string& output, std::uint64_t value)
{
    append(output, std::to_string(value));
}

void append(std::string& output, std::int32_t value)
{
    append(output, std::to_string(value));
}

void append(std::string& output, bool value)
{
    append(output, std::string(value ? "1" : "0"));
}

std::string canonical(const NativeTimerModifyOperationPayload& payload)
{
    if (!nativeTimerModifyOperationPayloadValid(payload)) return {};
    std::string output;
    append(output, std::string(nativeTimerModifyKindName(payload.kind)));
    append(output, payload.timerAssignmentId);
    append(output, payload.expectedAssignmentRevision);
    append(output, payload.expectedIntentRevision);
    append(output, payload.assignmentEpoch);
    append(output, payload.nativeTimerBindingId);
    append(output, payload.expectedBindingRevision);
    append(output, payload.backendId);
    append(output, payload.backendGeneration);
    append(output, payload.backendNativeTimerId);
    append(output, payload.expectedCurrentFingerprint);
    const auto& specification = payload.expectedSpecification;
    append(output, specification.channelId);
    append(output, specification.title);
    append(output, specification.directory);
    append(output, specification.day);
    append(output, specification.weekdays);
    append(output, normalizedHhmm(specification.startTime));
    append(output, normalizedHhmm(specification.endTime));
    append(output, specification.priority);
    append(output, specification.lifetime);
    append(output, specification.enabled);
    append(output, specification.vps);
    return output;
}

bool read(
    const std::string& input,
    std::size_t& position,
    std::string& value)
{
    if (position >= input.size()) return false;
    std::size_t length = 0;
    bool digitSeen = false;
    while (position < input.size() && input[position] != ':')
    {
        const char character = input[position++];
        if (character < '0' || character > '9') return false;
        digitSeen = true;
        const std::size_t digit =
            static_cast<std::size_t>(character - '0');
        if (length >
            (std::numeric_limits<std::size_t>::max() - digit) / 10)
            return false;
        length = length * 10 + digit;
    }
    if (!digitSeen || position >= input.size() ||
        input[position] != ':') return false;
    ++position;
    if (length > input.size() - position) return false;
    value = input.substr(position, length);
    position += length;
    if (position >= input.size() || input[position] != '|') return false;
    ++position;
    return true;
}

bool unsignedValue(const std::string& token, std::uint64_t& value)
{
    if (token.empty()) return false;
    value = 0;
    for (char character : token)
    {
        if (character < '0' || character > '9') return false;
        const std::uint64_t digit =
            static_cast<std::uint64_t>(character - '0');
        if (value >
            (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return value > 0;
}

bool smallInt(const std::string& token, std::int32_t& value)
{
    if (token.empty() || token.size() > 2) return false;
    int parsed = 0;
    for (char character : token)
    {
        if (character < '0' || character > '9') return false;
        parsed = parsed * 10 + character - '0';
    }
    if (parsed > 99) return false;
    value = parsed;
    return true;
}

bool booleanValue(const std::string& token, bool& value)
{
    if (token == "0") { value = false; return true; }
    if (token == "1") { value = true; return true; }
    return false;
}

bool kindValue(const std::string& token, NativeTimerModifyKind& kind)
{
    if (token == "update")
    {
        kind = NativeTimerModifyKind::update;
        return true;
    }
    if (token == "toggle")
    {
        kind = NativeTimerModifyKind::toggle;
        return true;
    }
    return false;
}
}

const char* nativeTimerModifyKindName(NativeTimerModifyKind kind)
{
    switch (kind)
    {
        case NativeTimerModifyKind::update: return "update";
        case NativeTimerModifyKind::toggle: return "toggle";
    }
    return "invalid";
}

bool nativeTimerModifyOperationPayloadValid(
    const NativeTimerModifyOperationPayload& payload)
{
    return std::string(nativeTimerModifyKindName(payload.kind)) != "invalid"
        && identity(payload.timerAssignmentId)
        && identity(payload.expectedAssignmentRevision)
        && identity(payload.expectedIntentRevision)
        && payload.assignmentEpoch > 0
        && identity(payload.nativeTimerBindingId)
        && identity(payload.expectedBindingRevision)
        && identity(payload.backendId)
        && payload.backendGeneration > 0
        && identity(payload.backendNativeTimerId)
        && !payload.expectedCurrentFingerprint.empty()
        && payload.expectedCurrentFingerprint.size() <= MaxFingerprint
        && nativeTimerSpecificationValid(payload.expectedSpecification);
}

std::string serializeNativeTimerModifyOperationPayload(
    const NativeTimerModifyOperationPayload& payload)
{
    const std::string body = canonical(payload);
    return body.empty() ? std::string() : std::string(PayloadPrefix) + body;
}

bool parseNativeTimerModifyOperationPayload(
    const std::string& serialized,
    NativeTimerModifyOperationPayload& payload)
{
    const std::string prefix(PayloadPrefix);
    if (serialized.compare(0, prefix.size(), prefix) != 0) return false;
    std::size_t position = prefix.size();
    std::vector<std::string> fields;
    while (position < serialized.size())
    {
        std::string field;
        if (!read(serialized, position, field)) return false;
        fields.push_back(std::move(field));
        if (fields.size() > 22) return false;
    }
    if (fields.size() != 22) return false;

    NativeTimerModifyOperationPayload parsed;
    if (!kindValue(fields[0], parsed.kind)) return false;
    parsed.timerAssignmentId = fields[1];
    parsed.expectedAssignmentRevision = fields[2];
    parsed.expectedIntentRevision = fields[3];
    if (!unsignedValue(fields[4], parsed.assignmentEpoch)) return false;
    parsed.nativeTimerBindingId = fields[5];
    parsed.expectedBindingRevision = fields[6];
    parsed.backendId = fields[7];
    if (!unsignedValue(fields[8], parsed.backendGeneration)) return false;
    parsed.backendNativeTimerId = fields[9];
    parsed.expectedCurrentFingerprint = fields[10];
    auto& specification = parsed.expectedSpecification;
    specification.channelId = fields[11];
    specification.title = fields[12];
    specification.directory = fields[13];
    specification.day = fields[14];
    specification.weekdays = fields[15];
    specification.startTime = fields[16];
    specification.endTime = fields[17];
    if (!smallInt(fields[18], specification.priority)
        || !smallInt(fields[19], specification.lifetime)
        || !booleanValue(fields[20], specification.enabled)
        || !booleanValue(fields[21], specification.vps))
        return false;
    if (!nativeTimerModifyOperationPayloadValid(parsed)
        || serializeNativeTimerModifyOperationPayload(parsed) != serialized)
        return false;
    payload = std::move(parsed);
    return true;
}

std::string nativeTimerModifyOperationPayloadFingerprint(
    const NativeTimerModifyOperationPayload& payload)
{
    const std::string body = canonical(payload);
    return body.empty()
        ? std::string()
        : std::string(FingerprintPrefix) + body;
}

}

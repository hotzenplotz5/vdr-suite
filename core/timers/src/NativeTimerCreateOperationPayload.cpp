#include "NativeTimerCreateOperationPayload.h"

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
constexpr std::size_t kMaxIdentityLength = 160;
constexpr const char* kPayloadPrefix =
    "native-timer-create-operation-payload/1|";
constexpr const char* kFingerprintPrefix =
    "native-timer-create-operation-payload-fingerprint/1|";

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

std::string normalizedHhmm(const std::string& value)
{
    return std::string(4 - value.size(), '0') + value;
}

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
    appendField(output, std::string(value ? "1" : "0"));
}

std::string canonicalBody(const NativeTimerCreateOperationPayload& payload)
{
    if (!nativeTimerCreateOperationPayloadValid(payload)) return {};

    std::string output;
    appendField(output, payload.timerAssignmentId);
    appendField(output, payload.expectedAssignmentRevision);
    appendField(output, payload.expectedIntentRevision);
    appendUnsigned(output, payload.assignmentEpoch);
    appendField(output, payload.nativeTimerBindingId);
    appendField(output, payload.backendId);
    appendUnsigned(output, payload.backendGeneration);

    const auto& specification = payload.expectedSpecification;
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
    return output;
}

bool readField(
    const std::string& input,
    std::size_t& position,
    std::string& value)
{
    if (position >= input.size()) return false;

    std::size_t length = 0;
    bool digitSeen = false;
    while (position < input.size() && input[position] != ':')
    {
        const char ch = input[position++];
        if (ch < '0' || ch > '9') return false;
        digitSeen = true;
        const std::size_t digit = static_cast<std::size_t>(ch - '0');
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

bool parseUnsigned(const std::string& token, std::uint64_t& value)
{
    if (token.empty()) return false;
    value = 0;
    for (char ch : token)
    {
        if (ch < '0' || ch > '9') return false;
        const std::uint64_t digit = static_cast<std::uint64_t>(ch - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return value > 0;
}

bool parseSmallInt(const std::string& token, std::int32_t& value)
{
    if (token.empty() || token.size() > 3) return false;
    int parsed = 0;
    for (char ch : token)
    {
        if (ch < '0' || ch > '9') return false;
        parsed = parsed * 10 + (ch - '0');
    }
    if (parsed < 0 || parsed > 99) return false;
    value = parsed;
    return true;
}

bool parseBool(const std::string& token, bool& value)
{
    if (token == "0")
    {
        value = false;
        return true;
    }
    if (token == "1")
    {
        value = true;
        return true;
    }
    return false;
}
}

bool nativeTimerCreateOperationPayloadValid(
    const NativeTimerCreateOperationPayload& payload)
{
    return validIdentity(payload.timerAssignmentId)
        && validIdentity(payload.expectedAssignmentRevision)
        && validIdentity(payload.expectedIntentRevision)
        && payload.assignmentEpoch > 0
        && validIdentity(payload.nativeTimerBindingId)
        && validIdentity(payload.backendId)
        && payload.backendGeneration > 0
        && nativeTimerSpecificationValid(payload.expectedSpecification);
}

std::string serializeNativeTimerCreateOperationPayload(
    const NativeTimerCreateOperationPayload& payload)
{
    const std::string body = canonicalBody(payload);
    if (body.empty()) return {};
    return std::string(kPayloadPrefix) + body;
}

bool parseNativeTimerCreateOperationPayload(
    const std::string& serialized,
    NativeTimerCreateOperationPayload& payload)
{
    const std::string prefix(kPayloadPrefix);
    if (serialized.compare(0, prefix.size(), prefix) != 0) return false;

    std::size_t position = prefix.size();
    std::vector<std::string> fields;
    fields.reserve(18);
    while (position < serialized.size())
    {
        std::string field;
        if (!readField(serialized, position, field)) return false;
        fields.push_back(std::move(field));
        if (fields.size() > 18) return false;
    }
    if (fields.size() != 18) return false;

    NativeTimerCreateOperationPayload parsed;
    parsed.timerAssignmentId = fields[0];
    parsed.expectedAssignmentRevision = fields[1];
    parsed.expectedIntentRevision = fields[2];
    if (!parseUnsigned(fields[3], parsed.assignmentEpoch)) return false;
    parsed.nativeTimerBindingId = fields[4];
    parsed.backendId = fields[5];
    if (!parseUnsigned(fields[6], parsed.backendGeneration)) return false;

    auto& specification = parsed.expectedSpecification;
    specification.channelId = fields[7];
    specification.title = fields[8];
    specification.directory = fields[9];
    specification.day = fields[10];
    specification.weekdays = fields[11];
    specification.startTime = fields[12];
    specification.endTime = fields[13];
    if (!parseSmallInt(fields[14], specification.priority)
        || !parseSmallInt(fields[15], specification.lifetime)
        || !parseBool(fields[16], specification.enabled)
        || !parseBool(fields[17], specification.vps))
    {
        return false;
    }

    if (!nativeTimerCreateOperationPayloadValid(parsed)) return false;
    if (serializeNativeTimerCreateOperationPayload(parsed) != serialized)
        return false;

    payload = std::move(parsed);
    return true;
}

std::string nativeTimerCreateOperationPayloadFingerprint(
    const NativeTimerCreateOperationPayload& payload)
{
    const std::string body = canonicalBody(payload);
    if (body.empty()) return {};
    return std::string(kFingerprintPrefix) + body;
}

}

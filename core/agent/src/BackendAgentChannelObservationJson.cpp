#include "BackendAgentChannelObservationJson.h"

#include "BackendAgentLifecycle.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr std::size_t MaximumBodyBytes = 512U * 1024U;
constexpr std::size_t MaximumArrayItems = 4096U;
constexpr std::size_t MaximumStringBytes = 4096U;
constexpr int MaximumDepth = 5;

enum class JsonKind { String, Unsigned, Boolean, Object, Array };

struct JsonValue
{
    JsonKind kind = JsonKind::String;
    std::string stringValue;
    std::uint64_t unsignedValue = 0;
    bool boolValue = false;
    std::map<std::string, JsonValue> objectValue;
    std::vector<JsonValue> arrayValue;
};

bool validUtf8(const std::string& value)
{
    std::size_t index = 0;
    while (index < value.size())
    {
        const unsigned char first = static_cast<unsigned char>(value[index]);
        if (first <= 0x7fU)
        {
            ++index;
            continue;
        }
        std::size_t count = 0;
        std::uint32_t codePoint = 0;
        if ((first & 0xe0U) == 0xc0U) { count = 2; codePoint = first & 0x1fU; }
        else if ((first & 0xf0U) == 0xe0U) { count = 3; codePoint = first & 0x0fU; }
        else if ((first & 0xf8U) == 0xf0U) { count = 4; codePoint = first & 0x07U; }
        else return false;
        if (index + count > value.size()) return false;
        for (std::size_t offset = 1; offset < count; ++offset)
        {
            const unsigned char next = static_cast<unsigned char>(value[index + offset]);
            if ((next & 0xc0U) != 0x80U) return false;
            codePoint = (codePoint << 6U) | (next & 0x3fU);
        }
        if ((count == 2 && codePoint < 0x80U) ||
            (count == 3 && codePoint < 0x800U) ||
            (count == 4 && codePoint < 0x10000U) ||
            codePoint > 0x10ffffU ||
            (codePoint >= 0xd800U && codePoint <= 0xdfffU))
        {
            return false;
        }
        index += count;
    }
    return true;
}

class JsonParser
{
public:
    explicit JsonParser(const std::string& input) : input_(input) {}

    bool parse(JsonValue& value)
    {
        position_ = skipSpace(position_);
        if (!parseValue(value, 0)) return false;
        position_ = skipSpace(position_);
        return position_ == input_.size();
    }

private:
    std::size_t skipSpace(std::size_t position) const
    {
        while (position < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position]))) ++position;
        return position;
    }

    bool parseValue(JsonValue& value, int depth)
    {
        if (depth > MaximumDepth || position_ >= input_.size()) return false;
        if (input_[position_] == '"')
        {
            value.kind = JsonKind::String;
            return parseString(value.stringValue);
        }
        if (input_[position_] == '{') return parseObject(value, depth + 1);
        if (input_[position_] == '[') return parseArray(value, depth + 1);
        if (input_.compare(position_, 4, "true") == 0)
        {
            value.kind = JsonKind::Boolean;
            value.boolValue = true;
            position_ += 4;
            return true;
        }
        if (input_.compare(position_, 5, "false") == 0)
        {
            value.kind = JsonKind::Boolean;
            value.boolValue = false;
            position_ += 5;
            return true;
        }
        return parseUnsigned(value);
    }

    bool parseString(std::string& value)
    {
        value.clear();
        if (position_ >= input_.size() || input_[position_] != '"') return false;
        ++position_;
        while (position_ < input_.size())
        {
            const unsigned char character = static_cast<unsigned char>(input_[position_++]);
            if (character == '"') return validUtf8(value);
            if (character < 0x20U) return false;
            if (character == '\\')
            {
                if (position_ >= input_.size()) return false;
                const char escaped = input_[position_++];
                switch (escaped)
                {
                    case '"': value.push_back('"'); break;
                    case '\\': value.push_back('\\'); break;
                    case '/': value.push_back('/'); break;
                    case 'b': value.push_back('\b'); break;
                    case 'f': value.push_back('\f'); break;
                    case 'n': value.push_back('\n'); break;
                    case 'r': value.push_back('\r'); break;
                    case 't': value.push_back('\t'); break;
                    default: return false;
                }
            }
            else value.push_back(static_cast<char>(character));
            if (value.size() > MaximumStringBytes) return false;
        }
        return false;
    }

    bool parseUnsigned(JsonValue& value)
    {
        const std::size_t start = position_;
        while (position_ < input_.size() &&
               std::isdigit(static_cast<unsigned char>(input_[position_]))) ++position_;
        if (start == position_ || position_ - start > 19 ||
            (position_ - start > 1 && input_[start] == '0')) return false;
        std::uint64_t number = 0;
        for (std::size_t index = start; index < position_; ++index)
        {
            const unsigned digit = static_cast<unsigned>(input_[index] - '0');
            if (number > (static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max()) - digit) / 10U)
                return false;
            number = number * 10U + digit;
        }
        value.kind = JsonKind::Unsigned;
        value.unsignedValue = number;
        return true;
    }

    bool parseObject(JsonValue& value, int depth)
    {
        value.kind = JsonKind::Object;
        value.objectValue.clear();
        ++position_;
        position_ = skipSpace(position_);
        if (position_ < input_.size() && input_[position_] == '}')
        {
            ++position_;
            return true;
        }
        while (position_ < input_.size())
        {
            std::string key;
            if (!parseString(key) || key.empty() || key.size() > 128) return false;
            position_ = skipSpace(position_);
            if (position_ >= input_.size() || input_[position_] != ':') return false;
            ++position_;
            position_ = skipSpace(position_);
            JsonValue child;
            if (!parseValue(child, depth)) return false;
            if (!value.objectValue.emplace(std::move(key), std::move(child)).second ||
                value.objectValue.size() > 32U) return false;
            position_ = skipSpace(position_);
            if (position_ >= input_.size()) return false;
            if (input_[position_] == '}')
            {
                ++position_;
                return true;
            }
            if (input_[position_] != ',') return false;
            ++position_;
            position_ = skipSpace(position_);
        }
        return false;
    }

    bool parseArray(JsonValue& value, int depth)
    {
        value.kind = JsonKind::Array;
        value.arrayValue.clear();
        ++position_;
        position_ = skipSpace(position_);
        if (position_ < input_.size() && input_[position_] == ']')
        {
            ++position_;
            return true;
        }
        while (position_ < input_.size())
        {
            JsonValue child;
            if (!parseValue(child, depth)) return false;
            value.arrayValue.push_back(std::move(child));
            if (value.arrayValue.size() > MaximumArrayItems) return false;
            position_ = skipSpace(position_);
            if (position_ >= input_.size()) return false;
            if (input_[position_] == ']')
            {
                ++position_;
                return true;
            }
            if (input_[position_] != ',') return false;
            ++position_;
            position_ = skipSpace(position_);
        }
        return false;
    }

    const std::string& input_;
    std::size_t position_ = 0;
};

bool exactKeys(
    const std::map<std::string, JsonValue>& object,
    const std::vector<std::string>& required)
{
    if (object.size() != required.size()) return false;
    return std::all_of(required.begin(), required.end(), [&](const std::string& key) {
        return object.find(key) != object.end();
    });
}

const JsonValue* valueFor(
    const std::map<std::string, JsonValue>& object,
    const std::string& key,
    JsonKind kind)
{
    const auto found = object.find(key);
    return found != object.end() && found->second.kind == kind ? &found->second : nullptr;
}

bool parseFact(const JsonValue& value, BackendAgentChannelFact& fact)
{
    if (value.kind != JsonKind::Object || !exactKeys(value.objectValue, {
            "channelId", "channelNumber", "name", "provider", "groupName",
            "radio", "encrypted", "enabled"})) return false;
    const JsonValue* channelId = valueFor(value.objectValue, "channelId", JsonKind::String);
    const JsonValue* channelNumber = valueFor(
        value.objectValue, "channelNumber", JsonKind::Unsigned);
    const JsonValue* name = valueFor(value.objectValue, "name", JsonKind::String);
    const JsonValue* provider = valueFor(value.objectValue, "provider", JsonKind::String);
    const JsonValue* groupName = valueFor(value.objectValue, "groupName", JsonKind::String);
    const JsonValue* radio = valueFor(value.objectValue, "radio", JsonKind::Boolean);
    const JsonValue* encrypted = valueFor(
        value.objectValue, "encrypted", JsonKind::Boolean);
    const JsonValue* enabled = valueFor(value.objectValue, "enabled", JsonKind::Boolean);
    if (channelId == nullptr || channelNumber == nullptr || name == nullptr ||
        provider == nullptr || groupName == nullptr || radio == nullptr ||
        encrypted == nullptr || enabled == nullptr) return false;
    fact.channelId = channelId->stringValue;
    fact.channelNumber = channelNumber->unsignedValue;
    fact.name = name->stringValue;
    fact.provider = provider->stringValue;
    fact.groupName = groupName->stringValue;
    fact.radio = radio->boolValue;
    fact.encrypted = encrypted->boolValue;
    fact.enabled = enabled->boolValue;
    return true;
}

bool parseFactArray(
    const JsonValue& value,
    std::vector<BackendAgentChannelFact>& facts)
{
    if (value.kind != JsonKind::Array) return false;
    facts.clear();
    facts.reserve(value.arrayValue.size());
    for (const JsonValue& item : value.arrayValue)
    {
        BackendAgentChannelFact fact;
        if (!parseFact(item, fact)) return false;
        facts.push_back(std::move(fact));
    }
    return true;
}

bool parseStringArray(const JsonValue& value, std::vector<std::string>& values)
{
    if (value.kind != JsonKind::Array) return false;
    values.clear();
    values.reserve(value.arrayValue.size());
    for (const JsonValue& item : value.arrayValue)
    {
        if (item.kind != JsonKind::String || item.stringValue.size() > 128) return false;
        values.push_back(item.stringValue);
    }
    return true;
}

std::string jsonEscape(const std::string& value)
{
    std::ostringstream output;
    for (unsigned char character : value)
    {
        switch (character)
        {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character >= 0x20U) output << static_cast<char>(character);
        }
    }
    return output.str();
}

std::string factJson(const BackendAgentChannelFact& fact)
{
    std::ostringstream output;
    output << "{\"channelId\":\"" << jsonEscape(fact.channelId)
           << "\",\"channelNumber\":" << fact.channelNumber
           << ",\"name\":\"" << jsonEscape(fact.name)
           << "\",\"provider\":\"" << jsonEscape(fact.provider)
           << "\",\"groupName\":\"" << jsonEscape(fact.groupName)
           << "\",\"radio\":" << (fact.radio ? "true" : "false")
           << ",\"encrypted\":" << (fact.encrypted ? "true" : "false")
           << ",\"enabled\":" << (fact.enabled ? "true" : "false") << '}';
    return output.str();
}
}

bool parseBackendAgentChannelObservationJson(
    const std::string& body,
    BackendAgentObservationRequest& request,
    std::string& reasonCode)
{
    request = BackendAgentObservationRequest{};
    if (body.empty() || body.size() > MaximumBodyBytes)
    {
        reasonCode = "channel_observation_payload_too_large";
        return false;
    }
    JsonValue root;
    JsonParser parser(body);
    if (!parser.parse(root) || root.kind != JsonKind::Object ||
        !exactKeys(root.objectValue, {
            "protocolVersion", "backendId", "agentInstanceId", "backendGeneration",
            "observationDomain", "snapshotGeneration", "producerSequence", "kind",
            "capturedAt", "resourceRevision", "observedHeartbeatSequence", "payload"}))
    {
        reasonCode = "invalid_channel_observation_json";
        return false;
    }
    const JsonValue* protocolVersion = valueFor(
        root.objectValue, "protocolVersion", JsonKind::String);
    const JsonValue* backendId = valueFor(root.objectValue, "backendId", JsonKind::String);
    const JsonValue* agentInstanceId = valueFor(
        root.objectValue, "agentInstanceId", JsonKind::String);
    const JsonValue* backendGeneration = valueFor(
        root.objectValue, "backendGeneration", JsonKind::Unsigned);
    const JsonValue* observationDomain = valueFor(
        root.objectValue, "observationDomain", JsonKind::String);
    const JsonValue* snapshotGeneration = valueFor(
        root.objectValue, "snapshotGeneration", JsonKind::Unsigned);
    const JsonValue* producerSequence = valueFor(
        root.objectValue, "producerSequence", JsonKind::Unsigned);
    const JsonValue* kind = valueFor(root.objectValue, "kind", JsonKind::String);
    const JsonValue* capturedAt = valueFor(root.objectValue, "capturedAt", JsonKind::Unsigned);
    const JsonValue* resourceRevision = valueFor(
        root.objectValue, "resourceRevision", JsonKind::String);
    const JsonValue* observedHeartbeatSequence = valueFor(
        root.objectValue, "observedHeartbeatSequence", JsonKind::Unsigned);
    const JsonValue* payload = valueFor(root.objectValue, "payload", JsonKind::Object);
    if (protocolVersion == nullptr || backendId == nullptr || agentInstanceId == nullptr ||
        backendGeneration == nullptr || observationDomain == nullptr ||
        snapshotGeneration == nullptr || producerSequence == nullptr || kind == nullptr ||
        capturedAt == nullptr || resourceRevision == nullptr ||
        observedHeartbeatSequence == nullptr || payload == nullptr ||
        observationDomain->stringValue != "channels" ||
        backendGeneration->unsignedValue > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        snapshotGeneration->unsignedValue > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        producerSequence->unsignedValue > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        capturedAt->unsignedValue > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        observedHeartbeatSequence->unsignedValue > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
    {
        reasonCode = "invalid_channel_observation_envelope";
        return false;
    }
    request.protocolVersion = protocolVersion->stringValue;
    request.backendId = backendId->stringValue;
    request.agentInstanceId = agentInstanceId->stringValue;
    request.backendGeneration = backendGeneration->unsignedValue;
    request.observationDomain = observationDomain->stringValue;
    request.snapshotGeneration = snapshotGeneration->unsignedValue;
    request.producerSequence = producerSequence->unsignedValue;
    request.kind = kind->stringValue;
    request.capturedAt = static_cast<std::int64_t>(capturedAt->unsignedValue);
    request.resourceRevision = resourceRevision->stringValue;
    request.observedHeartbeatSequence = observedHeartbeatSequence->unsignedValue;
    if (request.kind == "completeSnapshot")
    {
        if (!exactKeys(payload->objectValue, {"channels"}) ||
            !parseFactArray(payload->objectValue.at("channels"), request.channels))
        {
            reasonCode = "invalid_channel_snapshot_payload";
            return false;
        }
    }
    else if (request.kind == "changeBatch")
    {
        if (!exactKeys(payload->objectValue, {"upserts", "removedChannelIds"}) ||
            !parseFactArray(payload->objectValue.at("upserts"), request.upserts) ||
            !parseStringArray(
                payload->objectValue.at("removedChannelIds"),
                request.removedChannelIds))
        {
            reasonCode = "invalid_channel_change_payload";
            return false;
        }
    }
    else
    {
        reasonCode = "invalid_channel_observation_kind";
        return false;
    }
    reasonCode = "channel_observation_parsed";
    return true;
}

std::string serializeBackendAgentChannelObservationJson(
    const BackendAgentObservationRequest& request)
{
    if (request.observationDomain != "channels" ||
        (request.kind != "completeSnapshot" && request.kind != "changeBatch")) return {};
    std::ostringstream output;
    output << "{\"protocolVersion\":\"" << jsonEscape(request.protocolVersion)
           << "\",\"backendId\":\"" << jsonEscape(request.backendId)
           << "\",\"agentInstanceId\":\"" << jsonEscape(request.agentInstanceId)
           << "\",\"backendGeneration\":" << request.backendGeneration
           << ",\"observationDomain\":\"channels\""
           << ",\"snapshotGeneration\":" << request.snapshotGeneration
           << ",\"producerSequence\":" << request.producerSequence
           << ",\"kind\":\"" << request.kind
           << "\",\"capturedAt\":" << request.capturedAt
           << ",\"resourceRevision\":\"" << jsonEscape(request.resourceRevision)
           << "\",\"observedHeartbeatSequence\":"
           << request.observedHeartbeatSequence << ",\"payload\":";
    if (request.kind == "completeSnapshot")
    {
        output << "{\"channels\":[";
        for (std::size_t index = 0; index < request.channels.size(); ++index)
        {
            if (index != 0) output << ',';
            output << factJson(request.channels[index]);
        }
        output << "]}";
    }
    else
    {
        output << "{\"upserts\":[";
        for (std::size_t index = 0; index < request.upserts.size(); ++index)
        {
            if (index != 0) output << ',';
            output << factJson(request.upserts[index]);
        }
        output << "],\"removedChannelIds\":[";
        for (std::size_t index = 0; index < request.removedChannelIds.size(); ++index)
        {
            if (index != 0) output << ',';
            output << '"' << jsonEscape(request.removedChannelIds[index]) << '"';
        }
        output << "]}";
    }
    output << '}';
    return output.str();
}

#include "PublicTimerCreateRequestParser.h"

#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>\n#include <utility>

namespace
{
enum class JsonValueKind { stringValue, integerValue, boolValue, objectValue };

struct JsonValue
{
    JsonValueKind kind = JsonValueKind::stringValue;
    std::string stringValue;
    std::int64_t integerValue = 0;
    bool boolValue = false;
    std::map<std::string, JsonValue> objectValue;
};

class Parser
{
public:
    explicit Parser(const std::string& input) : input_(input) {}

    bool parseRoot(JsonValue& value)
    {
        skipWhitespace();
        if (!parseObject(value)) return false;
        skipWhitespace();
        return position_ == input_.size();
    }

private:
    void skipWhitespace()
    {
        while (position_ < input_.size() &&
            std::isspace(static_cast<unsigned char>(input_[position_])))
        {
            ++position_;
        }
    }

    bool parseString(std::string& value)
    {
        if (position_ >= input_.size() || input_[position_] != '"') return false;
        ++position_;
        value.clear();
        while (position_ < input_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(input_[position_++]);
            if (character == '"') return true;
            if (character < 0x20U) return false;
            if (character != '\\')
            {
                value.push_back(static_cast<char>(character));
                continue;
            }
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
        return false;
    }

    bool parseInteger(std::int64_t& value)
    {
        bool negative = false;
        if (position_ < input_.size() && input_[position_] == '-')
        {
            negative = true;
            ++position_;
        }
        if (position_ >= input_.size() ||
            !std::isdigit(static_cast<unsigned char>(input_[position_])))
        {
            return false;
        }
        std::uint64_t parsed = 0;
        while (position_ < input_.size() &&
            std::isdigit(static_cast<unsigned char>(input_[position_])))
        {
            const std::uint64_t digit =
                static_cast<std::uint64_t>(input_[position_] - '0');
            if (parsed >
                (static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max()) - digit) / 10U)
            {
                return false;
            }
            parsed = parsed * 10U + digit;
            ++position_;
        }
        if (position_ < input_.size() &&
            (input_[position_] == '.' ||
             input_[position_] == 'e' ||
             input_[position_] == 'E'))
        {
            return false;
        }
        value = negative
            ? -static_cast<std::int64_t>(parsed)
            : static_cast<std::int64_t>(parsed);
        return true;
    }

    bool parseValue(JsonValue& value)
    {
        skipWhitespace();
        if (position_ >= input_.size()) return false;
        if (input_[position_] == '{') return parseObject(value);
        if (input_[position_] == '"')
        {
            value.kind = JsonValueKind::stringValue;
            return parseString(value.stringValue);
        }
        if (input_.compare(position_, 4, "true") == 0)
        {
            position_ += 4;
            value.kind = JsonValueKind::boolValue;
            value.boolValue = true;
            return true;
        }
        if (input_.compare(position_, 5, "false") == 0)
        {
            position_ += 5;
            value.kind = JsonValueKind::boolValue;
            value.boolValue = false;
            return true;
        }
        value.kind = JsonValueKind::integerValue;
        return parseInteger(value.integerValue);
    }

    bool parseObject(JsonValue& value)
    {
        if (position_ >= input_.size() || input_[position_] != '{') return false;
        ++position_;
        value.kind = JsonValueKind::objectValue;
        value.objectValue.clear();
        skipWhitespace();
        if (position_ < input_.size() && input_[position_] == '}')
        {
            ++position_;
            return true;
        }
        for (;;)
        {
            skipWhitespace();
            std::string key;
            if (!parseString(key) || key.empty()) return false;
            if (value.objectValue.find(key) != value.objectValue.end()) return false;
            skipWhitespace();
            if (position_ >= input_.size() || input_[position_] != ':') return false;
            ++position_;
            JsonValue child;
            if (!parseValue(child)) return false;
            value.objectValue.emplace(key, std::move(child));
            skipWhitespace();
            if (position_ >= input_.size()) return false;
            if (input_[position_] == '}')
            {
                ++position_;
                return true;
            }
            if (input_[position_] != ',') return false;
            ++position_;
        }
    }

    const std::string& input_;
    std::size_t position_ = 0;
};

PublicTimerCreateRequestParseResult result(
    PublicTimerCreateRequestParseStatus status,
    const std::string& detail)
{
    PublicTimerCreateRequestParseResult value;
    value.status = status;
    value.detail = detail;
    return value;
}

bool exactKeys(
    const std::map<std::string, JsonValue>& object,
    const std::set<std::string>& expected)
{
    if (object.size() != expected.size()) return false;
    for (const auto& entry : object)
    {
        if (expected.find(entry.first) == expected.end()) return false;
    }
    return true;
}

bool hhmm(const std::string& value)
{
    if (value.size() != 4U) return false;
    for (unsigned char character : value)
        if (!std::isdigit(character)) return false;
    const int hours =
        (value[0] - '0') * 10 + (value[1] - '0');
    const int minutes =
        (value[2] - '0') * 10 + (value[3] - '0');
    return hours >= 0 && hours <= 23 &&
        minutes >= 0 && minutes <= 59;
}

bool boundedText(
    const std::string& value,
    std::size_t maximum,
    bool allowEmpty)
{
    if ((!allowEmpty && value.empty()) || value.size() > maximum) return false;
    for (unsigned char character : value)
        if (character < 0x20U || character == 0x7fU) return false;
    return true;
}
}

PublicTimerCreateRequestParseResult
PublicTimerCreateRequestParser::parse(const std::string& body) const
{
    JsonValue root;
    Parser parser(body);
    if (!parser.parseRoot(root))
        return result(
            PublicTimerCreateRequestParseStatus::invalidJson,
            "The request body is not valid JSON.");

    if (root.kind != JsonValueKind::objectValue ||
        !exactKeys(root.objectValue, {"nativeTimer"}))
    {
        return result(
            PublicTimerCreateRequestParseStatus::invalidRequest,
            "The request body must contain only the nativeTimer object.");
    }

    const JsonValue& nativeTimer = root.objectValue.at("nativeTimer");
    if (nativeTimer.kind != JsonValueKind::objectValue)
    {
        return result(
            PublicTimerCreateRequestParseStatus::invalidRequest,
            "nativeTimer must be an object.");
    }

    const std::set<std::string> fields = {
        "title", "directory", "day", "weekdays",
        "startTime", "endTime", "priority", "lifetime",
        "enabled", "vps"
    };
    if (!exactKeys(nativeTimer.objectValue, fields))
    {
        return result(
            PublicTimerCreateRequestParseStatus::invalidRequest,
            "nativeTimer must contain exactly the documented fields.");
    }

    const auto stringField =
        [&](const std::string& name, std::string& target) -> bool
        {
            const auto found = nativeTimer.objectValue.find(name);
            if (found == nativeTimer.objectValue.end() ||
                found->second.kind != JsonValueKind::stringValue)
            {
                return false;
            }
            target = found->second.stringValue;
            return true;
        };
    const auto integerField =
        [&](const std::string& name, std::int32_t& target) -> bool
        {
            const auto found = nativeTimer.objectValue.find(name);
            if (found == nativeTimer.objectValue.end() ||
                found->second.kind != JsonValueKind::integerValue ||
                found->second.integerValue <
                    std::numeric_limits<std::int32_t>::min() ||
                found->second.integerValue >
                    std::numeric_limits<std::int32_t>::max())
            {
                return false;
            }
            target = static_cast<std::int32_t>(found->second.integerValue);
            return true;
        };
    const auto boolField =
        [&](const std::string& name, bool& target) -> bool
        {
            const auto found = nativeTimer.objectValue.find(name);
            if (found == nativeTimer.objectValue.end() ||
                found->second.kind != JsonValueKind::boolValue)
            {
                return false;
            }
            target = found->second.boolValue;
            return true;
        };

    PublicTimerCreateRequestParseResult parsed;
    parsed.status = PublicTimerCreateRequestParseStatus::ok;
    if (!stringField("title", parsed.specification.title) ||
        !stringField("directory", parsed.specification.directory) ||
        !stringField("day", parsed.specification.day) ||
        !stringField("weekdays", parsed.specification.weekdays) ||
        !stringField("startTime", parsed.specification.startTime) ||
        !stringField("endTime", parsed.specification.endTime) ||
        !integerField("priority", parsed.specification.priority) ||
        !integerField("lifetime", parsed.specification.lifetime) ||
        !boolField("enabled", parsed.specification.enabled) ||
        !boolField("vps", parsed.specification.vps))
    {
        return result(
            PublicTimerCreateRequestParseStatus::invalidRequest,
            "nativeTimer contains a field with the wrong JSON type.");
    }

    if (!boundedText(parsed.specification.title, 1024U, true) ||
        !boundedText(parsed.specification.directory, 1024U, true) ||
        !boundedText(parsed.specification.day, 160U, false) ||
        parsed.specification.weekdays.size() != 7U ||
        !hhmm(parsed.specification.startTime) ||
        !hhmm(parsed.specification.endTime) ||
        parsed.specification.priority < 0 ||
        parsed.specification.priority > 99 ||
        parsed.specification.lifetime < 0 ||
        parsed.specification.lifetime > 99)
    {
        return result(
            PublicTimerCreateRequestParseStatus::validationError,
            "nativeTimer contains invalid Timer values.");
    }

    for (unsigned char character : parsed.specification.weekdays)
    {
        if (character != '-' && std::isalpha(character) == 0)
        {
            return result(
                PublicTimerCreateRequestParseStatus::validationError,
                "nativeTimer.weekdays is invalid.");
        }
    }

    return parsed;
}

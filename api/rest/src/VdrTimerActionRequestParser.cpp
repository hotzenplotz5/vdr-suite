#include "VdrTimerActionRequestParser.h"

#include <cstdlib>
#include <map>
#include <string>

namespace
{
// Decode the browser's JSON.stringify values without interpreting commas,
// closing braces, or escaped quote characters inside a Timer title/Aux blob
// as top-level object delimiters.
bool parseHexQuad(const std::string& body, std::size_t& pos, unsigned& code)
{
    if (body.size() - pos < 4U) return false;
    code = 0U;
    for (unsigned i = 0U; i < 4U; ++i)
    {
        const char c = body[pos++];
        const int digit =
            (c >= '0' && c <= '9') ? c - '0' :
            (c >= 'a' && c <= 'f') ? c - 'a' + 10 :
            (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
        if (digit < 0) return false;
        code = (code << 4U) | static_cast<unsigned>(digit);
    }
    return true;
}

void appendUtf8(std::string& output, unsigned code)
{
    if (code < 0x80U)
        output.push_back(static_cast<char>(code));
    else if (code < 0x800U)
    {
        output.push_back(static_cast<char>(0xc0U | (code >> 6U)));
        output.push_back(static_cast<char>(0x80U | (code & 0x3fU)));
    }
    else if (code < 0x10000U)
    {
        output.push_back(static_cast<char>(0xe0U | (code >> 12U)));
        output.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (code & 0x3fU)));
    }
    else
    {
        output.push_back(static_cast<char>(0xf0U | (code >> 18U)));
        output.push_back(static_cast<char>(0x80U | ((code >> 12U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (code & 0x3fU)));
    }
}

bool parseJsonString(
    const std::string& body, std::size_t& pos, std::string& output)
{
    if (pos >= body.size() || body[pos++] != '"') return false;
    output.clear();
    while (pos < body.size())
    {
        const unsigned char c = static_cast<unsigned char>(body[pos++]);
        if (c == '"') return true;
        if (c < 0x20U) return false;
        if (c != '\\')
        {
            output.push_back(static_cast<char>(c));
            continue;
        }
        if (pos >= body.size()) return false;
        const char escape = body[pos++];
        switch (escape)
        {
        case '"': output.push_back('"'); break;
        case '\\': output.push_back('\\'); break;
        case '/': output.push_back('/'); break;
        case 'b': output.push_back('\b'); break;
        case 'f': output.push_back('\f'); break;
        case 'n': output.push_back('\n'); break;
        case 'r': output.push_back('\r'); break;
        case 't': output.push_back('\t'); break;
        case 'u':
        {
            unsigned code = 0U;
            if (!parseHexQuad(body, pos, code)) return false;
            if (code >= 0xd800U && code <= 0xdbffU)
            {
                if (body.size() - pos < 6U ||
                    body[pos] != '\\' || body[pos + 1U] != 'u')
                    return false;
                pos += 2U;
                unsigned low = 0U;
                if (!parseHexQuad(body, pos, low) ||
                    low < 0xdc00U || low > 0xdfffU)
                    return false;
                code = 0x10000U +
                    ((code - 0xd800U) << 10U) + (low - 0xdc00U);
            }
            else if (code >= 0xdc00U && code <= 0xdfffU)
                return false;
            appendUtf8(output, code);
            break;
        }
        default: return false;
        }
    }
    return false;
}

void skipWhitespace(const std::string& body, std::size_t& pos)
{
    while (pos < body.size() &&
        (body[pos] == ' ' || body[pos] == '\t' ||
         body[pos] == '\n' || body[pos] == '\r'))
        ++pos;
}

std::string trim(const std::string& value)
{
    const std::size_t first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    const std::size_t last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

std::map<std::string, std::string> parseFlatObject(
    const std::string& body)
{
    std::map<std::string, std::string> values;
    std::size_t pos = 0U;
    skipWhitespace(body, pos);
    if (pos >= body.size() || body[pos++] != '{') return {};
    skipWhitespace(body, pos);
    if (pos < body.size() && body[pos] == '}')
    {
        ++pos;
        skipWhitespace(body, pos);
        return pos == body.size() ? values :
            std::map<std::string, std::string>{};
    }
    while (pos < body.size())
    {
        std::string key;
        if (!parseJsonString(body, pos, key)) return {};
        skipWhitespace(body, pos);
        if (pos >= body.size() || body[pos++] != ':') return {};
        skipWhitespace(body, pos);
        std::string value;
        if (pos < body.size() && body[pos] == '"')
        {
            if (!parseJsonString(body, pos, value)) return {};
        }
        else
        {
            const std::size_t first = pos;
            while (pos < body.size() &&
                   body[pos] != ',' && body[pos] != '}')
                ++pos;
            value = trim(body.substr(first, pos - first));
            if (value.empty()) return {};
        }
        values[key] = value;
        skipWhitespace(body, pos);
        if (pos < body.size() && body[pos] == '}')
        {
            ++pos;
            skipWhitespace(body, pos);
            return pos == body.size() ? values :
                std::map<std::string, std::string>{};
        }
        if (pos >= body.size() || body[pos++] != ',') return {};
        skipWhitespace(body, pos);
    }
    return {};
}

std::string getValue(
    const std::map<std::string, std::string>& values,
    const std::string& key)
{
    const auto iterator =
        values.find(key);

    if (iterator == values.end())
    {
        return "";
    }

    return iterator->second;
}

bool parseBool(
    const std::map<std::string, std::string>& values,
    const std::string& key,
    bool defaultValue)
{
    const auto iterator =
        values.find(key);

    if (iterator == values.end())
    {
        return defaultValue;
    }

    return iterator->second == "true" ||
           iterator->second == "1";
}

int parseInt(
    const std::map<std::string, std::string>& values,
    const std::string& key,
    int defaultValue)
{
    const auto iterator =
        values.find(key);

    if (iterator == values.end() ||
        iterator->second.empty())
    {
        return defaultValue;
    }

    return std::atoi(iterator->second.c_str());
}
}

VdrTimerOperationRequest VdrTimerActionRequestParser::parse(
    const std::string& body) const
{
    const auto values =
        parseFlatObject(body);

    VdrTimerOperationRequest request;

    request.backendId =
        getValue(values, "backendId");

    request.timerId =
        getValue(values, "timerId");

    request.channelId =
        getValue(values, "channelId");

    request.title =
        getValue(values, "title");

    request.directory =
        getValue(values, "directory");

    request.day =
        getValue(values, "day");

    request.weekdays =
        getValue(values, "weekdays");

    if (request.weekdays.empty())
    {
        request.weekdays = "-------";
    }

    request.start =
        parseInt(values, "start", 0);

    request.stop =
        parseInt(values, "stop", 0);

    request.priority =
        parseInt(values, "priority", 50);

    request.lifetime =
        parseInt(values, "lifetime", 99);

    request.active =
        parseBool(values, "active", true);

    request.vps =
        parseBool(values, "vps", false);

    request.aux =
        getValue(values, "aux");

    return request;
}

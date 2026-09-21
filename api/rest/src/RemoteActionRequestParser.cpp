#include "RemoteActionRequestParser.h"

#include "JsonStringDecoder.h"

#include <cctype>
#include <map>

namespace
{
std::size_t skipWhitespace(const std::string& value, std::size_t position)
{
    while (position < value.size() &&
           std::isspace(static_cast<unsigned char>(value[position])))
    {
        ++position;
    }

    return position;
}

bool parseJsonString(
    const std::string& body,
    std::size_t& position,
    std::string& value)
{
    position = skipWhitespace(body, position);

    if (position >= body.size() || body[position] != '"')
    {
        return false;
    }

    const std::size_t start = ++position;
    bool escaped = false;

    while (position < body.size())
    {
        const char character = body[position];

        if (escaped)
        {
            escaped = false;
            ++position;
            continue;
        }

        if (character == '\\')
        {
            escaped = true;
            ++position;
            continue;
        }

        if (character == '"')
        {
            value = vdrsuite::decodeJsonStringEscapes(
                body.substr(start, position - start));
            ++position;
            return true;
        }

        ++position;
    }

    return false;
}

bool parseFlatStringObject(
    const std::string& body,
    std::map<std::string, std::string>& values)
{
    std::size_t position = skipWhitespace(body, 0);

    if (position >= body.size() || body[position] != '{')
    {
        return false;
    }

    ++position;
    position = skipWhitespace(body, position);

    if (position < body.size() && body[position] == '}')
    {
        ++position;
        return skipWhitespace(body, position) == body.size();
    }

    while (position < body.size())
    {
        std::string key;
        std::string value;

        if (!parseJsonString(body, position, key))
        {
            return false;
        }

        position = skipWhitespace(body, position);

        if (position >= body.size() || body[position] != ':')
        {
            return false;
        }

        ++position;

        if (!parseJsonString(body, position, value))
        {
            return false;
        }

        values[key] = value;
        position = skipWhitespace(body, position);

        if (position >= body.size())
        {
            return false;
        }

        if (body[position] == '}')
        {
            ++position;
            return skipWhitespace(body, position) == body.size();
        }

        if (body[position] != ',')
        {
            return false;
        }

        ++position;
    }

    return false;
}

std::string valueOf(
    const std::map<std::string, std::string>& values,
    const std::string& key)
{
    const auto iterator = values.find(key);
    return iterator == values.end() ? "" : iterator->second;
}
}

RemoteActionParseResult RemoteActionRequestParser::parse(
    const std::string& body) const
{
    RemoteActionParseResult result;
    std::map<std::string, std::string> values;

    result.validJson = parseFlatStringObject(body, values);

    if (!result.validJson)
    {
        result.errors.push_back(
            "request body must be a flat JSON object with string values");
        return result;
    }

    result.request.backendId = valueOf(values, "backendId");
    result.request.operationId = valueOf(values, "operationId");
    result.request.channelId = valueOf(values, "channelId");
    result.request.action = remoteActionTypeFromName(
        valueOf(values, "action"));

    return result;
}

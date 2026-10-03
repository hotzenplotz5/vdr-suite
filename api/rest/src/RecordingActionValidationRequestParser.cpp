#include "RecordingActionValidationRequestParser.h"

#include "RecordingActionUtils.h"

#include <map>
#include <string>

namespace
{
void skipWhitespace(const std::string& text, std::size_t& position)
{
    while (position < text.size())
    {
        const char character = text[position];
        if (character != ' ' && character != '\t' &&
            character != '\n' && character != '\r')
        {
            break;
        }
        ++position;
    }
}

bool parseJsonString(
    const std::string& text,
    std::size_t& position,
    std::string& value)
{
    value.clear();
    if (position >= text.size() || text[position] != '"')
    {
        return false;
    }

    ++position;
    while (position < text.size())
    {
        const char character = text[position++];
        if (character == '"')
        {
            return true;
        }

        if (character != '\\')
        {
            value.push_back(character);
            continue;
        }

        if (position >= text.size())
        {
            return false;
        }

        const char escaped = text[position++];
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
        default:
            return false;
        }
    }

    return false;
}

std::string trim(
    const std::string& value)
{
    const std::size_t first =
        value.find_first_not_of(" \t\n\r");

    if (first == std::string::npos)
    {
        return "";
    }

    const std::size_t last =
        value.find_last_not_of(" \t\n\r");

    return value.substr(first, last - first + 1);
}

std::map<std::string, std::string> parseFlatObject(
    const std::string& body)
{
    std::map<std::string, std::string> values;
    std::size_t position = 0;
    skipWhitespace(body, position);

    if (position >= body.size() || body[position] != '{')
    {
        return {};
    }
    ++position;

    while (position < body.size())
    {
        skipWhitespace(body, position);
        if (position < body.size() && body[position] == '}')
        {
            ++position;
            skipWhitespace(body, position);
            return position == body.size() ? values
                                           : std::map<std::string, std::string>{};
        }

        std::string key;
        if (!parseJsonString(body, position, key))
        {
            return {};
        }

        skipWhitespace(body, position);
        if (position >= body.size() || body[position] != ':')
        {
            return {};
        }
        ++position;
        skipWhitespace(body, position);

        std::string value;
        if (position < body.size() && body[position] == '"')
        {
            if (!parseJsonString(body, position, value))
            {
                return {};
            }
        }
        else
        {
            const std::size_t valueStart = position;
            while (position < body.size() &&
                   body[position] != ',' &&
                   body[position] != '}')
            {
                ++position;
            }
            value = trim(body.substr(valueStart, position - valueStart));
        }

        if (!key.empty())
        {
            values[key] = value;
        }

        skipWhitespace(body, position);
        if (position >= body.size())
        {
            return {};
        }

        if (body[position] == ',')
        {
            ++position;
            continue;
        }

        if (body[position] == '}')
        {
            ++position;
            skipWhitespace(body, position);
            return position == body.size() ? values
                                           : std::map<std::string, std::string>{};
        }

        return {};
    }

    return {};
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
}

RecordingActionRequest RecordingActionValidationRequestParser::parse(
    const std::string& body) const
{
    const auto values =
        parseFlatObject(body);

    RecordingActionRequest request;

    request.backendId =
        getValue(values, "backendId");

    request.recordingId =
        getValue(values, "recordingId");

    request.type =
        fromString(getValue(values, "action"));

    request.dryRun =
        parseBool(values, "dryRun", true);

    const std::string targetPath =
        getValue(values, "targetPath");

    if (!targetPath.empty())
    {
        request.parameters["targetPath"] = targetPath;
    }

    const std::string newName =
        getValue(values, "newName");

    if (!newName.empty())
    {
        request.parameters["newName"] = newName;
    }

    const std::string recordingPath =
        getValue(values, "recordingPath");

    if (!recordingPath.empty())
    {
        request.parameters["recordingPath"] = recordingPath;
    }

    const std::string backendNativeId =
        getValue(values, "backendNativeId");

    if (!backendNativeId.empty())
    {
        request.parameters["backendNativeId"] = backendNativeId;
    }

    const std::string recordingTitle =
        getValue(values, "recordingTitle");

    if (!recordingTitle.empty())
    {
        request.parameters["recordingTitle"] = recordingTitle;
    }

    return request;
}

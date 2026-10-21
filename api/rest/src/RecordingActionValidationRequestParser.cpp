#include "RecordingActionValidationRequestParser.h"

#include "RecordingActionUtils.h"

#include <algorithm>
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
    const std::string& body, bool strictPublic = false)
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
            if (strictPublic && key == "dryRun")
                return {};
            if (!parseJsonString(body, position, value))
                return {};
        }
        else
        {
            const std::size_t valueStart = position;
            while (position < body.size() &&
                   body[position] != ',' &&
                   body[position] != '}')
                ++position;
            value = trim(body.substr(valueStart, position - valueStart));
            if (strictPublic &&
                (key != "dryRun" || (value != "true" && value != "false")))
                return {};
        }

        if (strictPublic && (key.empty() || values.count(key) != 0U))
            return {};
        if (!key.empty())
            values[key] = value;

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


bool RecordingActionValidationRequestParser::parsePublic(
    const std::string& body, RecordingActionRequest& request) const
{
    request = RecordingActionRequest{};
    if (body.size() > 4096U)
        return false;

    const auto values = parseFlatObject(body, true);
    if (values.size() < 3U)
        return false;
    for (const auto& item : values)
    {
        if (item.first != "backendId" && item.first != "recordingId" &&
            item.first != "action" && item.first != "dryRun" &&
            item.first != "newName" && item.first != "targetFolderId")
            return false;
    }

    const std::string backend = getValue(values, "backendId");
    const std::string id = getValue(values, "recordingId");
    const std::string action = getValue(values, "action");
    if (backend.empty() || backend.size() > 128U ||
        !std::all_of(backend.begin(), backend.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        }) ||
        id.size() != 36U || id.compare(0, 4, "rec_") != 0 ||
        !std::all_of(id.begin() + 4, id.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        }) ||
        (action != "RENAME" && action != "MOVE" && action != "DELETE"))
        return false;

    request.backendId = backend;
    request.recordingId = id;
    request.type = fromString(action);
    request.dryRun = parseBool(values, "dryRun", true);

    // Public rename is a single display-name component, not a path.
    const std::string newName = getValue(values, "newName");
    if ((!newName.empty() && action != "RENAME") ||
        newName.size() > 255U ||
        newName == "." || newName == ".." ||
        newName.find('/') != std::string::npos ||
        newName.find('\\') != std::string::npos ||
        newName.find('~') != std::string::npos ||
        std::any_of(newName.begin(), newName.end(), [](unsigned char c) {
            return c < 0x20 || c == 0x7f;
        }))
        return false;
    if (!newName.empty())
        request.parameters["newName"] = newName;

    // R3: only opaque folder identities from the existing Recording browse
    // hierarchy are accepted. Client targetPath is never a Public-v1 field.
    const std::string folderId = getValue(values, "targetFolderId");
    if (action == "MOVE")
    {
        if (folderId.size() != 37U ||
            folderId.compare(0, 5U, "fld1_") != 0 ||
            !std::all_of(folderId.begin() + 5, folderId.end(),
                [](unsigned char c) {
                    return (c >= '0' && c <= '9') ||
                           (c >= 'a' && c <= 'f');
                }))
            return false;
        request.parameters["publicTargetFolderId"] = folderId;
    }
    else if (!folderId.empty())
    {
        return false;
    }

    return true;
}

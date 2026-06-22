#include "SearchTimerCreateRequestParser.h"

#include <cstdlib>
#include <map>
#include <string>

namespace
{
std::string unquote(const std::string& value)
{
    if (value.size() >= 2 &&
        value.front() == '"' &&
        value.back() == '"')
    {
        return value.substr(1, value.size() - 2);
    }

    return value;
}

std::string trim(const std::string& value)
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

    while (position < body.size())
    {
        const std::size_t keyStart =
            body.find('"', position);

        if (keyStart == std::string::npos)
        {
            break;
        }

        const std::size_t keyEnd =
            body.find('"', keyStart + 1);

        if (keyEnd == std::string::npos)
        {
            break;
        }

        const std::size_t colon =
            body.find(':', keyEnd + 1);

        if (colon == std::string::npos)
        {
            break;
        }

        const std::string key =
            body.substr(keyStart + 1, keyEnd - keyStart - 1);

        const std::size_t comma =
            body.find(',', colon + 1);

        const std::size_t objectEnd =
            body.find('}', colon + 1);

        std::size_t valueEnd =
            body.size();

        if (comma != std::string::npos)
        {
            valueEnd = comma;
        }

        if (objectEnd != std::string::npos &&
            objectEnd < valueEnd)
        {
            valueEnd = objectEnd;
        }

        const std::string rawValue =
            trim(body.substr(colon + 1, valueEnd - colon - 1));

        if (!key.empty())
        {
            values[key] = unquote(rawValue);
        }

        if (comma == std::string::npos)
        {
            break;
        }

        position = comma + 1;
    }

    return values;
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

SearchTimerCreateRequest SearchTimerCreateRequestParser::parse(
    const std::string& body) const
{
    const auto values =
        parseFlatObject(body);

    SearchTimerCreateRequest request;

    request.backendId =
        getValue(values, "backendId").empty()
            ? "default"
            : getValue(values, "backendId");

    request.name =
        getValue(values, "name");

    request.query =
        getValue(values, "query");

    request.active =
        parseBool(values, "active", true);

    request.directory =
        getValue(values, "directory");

    request.priority =
        parseInt(values, "priority", 0);

    request.lifetime =
        parseInt(values, "lifetime", 0);

    request.marginStartMinutes =
        parseInt(values, "marginStartMinutes", 0);

    request.marginStopMinutes =
        parseInt(values, "marginStopMinutes", 0);

    request.useVps =
        parseBool(values, "useVps", false);

    request.useChannel =
        parseInt(values, "useChannel", 0);

    request.channels =
        getValue(values, "channels");

    request.channelMin =
        getValue(values, "channelMin");

    request.channelMax =
        getValue(values, "channelMax");

    request.useTime =
        parseBool(values, "useTime", false);

    request.startTime =
        parseInt(values, "startTime", 0);

    request.stopTime =
        parseInt(values, "stopTime", 0);

    request.useDuration =
        parseBool(values, "useDuration", false);

    request.durationMinMinutes =
        parseInt(values, "durationMinMinutes", 0);

    request.durationMaxMinutes =
        parseInt(values, "durationMaxMinutes", 0);

    request.useDayOfWeek =
        parseBool(values, "useDayOfWeek", false);

    request.dayOfWeek =
        parseInt(values, "dayOfWeek", 0);

    return request;
}

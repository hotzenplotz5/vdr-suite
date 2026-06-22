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

std::size_t findValueEnd(
    const std::string& body,
    const std::size_t valueStart)
{
    bool insideString = false;
    bool escaped = false;

    for (std::size_t index = valueStart; index < body.size(); ++index)
    {
        const char current =
            body[index];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (current == '\\' &&
            insideString)
        {
            escaped = true;
            continue;
        }

        if (current == '"')
        {
            insideString = !insideString;
            continue;
        }

        if (!insideString &&
            (current == ',' || current == '}'))
        {
            return index;
        }
    }

    return body.size();
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

        const std::size_t valueEnd =
            findValueEnd(body, colon + 1);

        const std::string rawValue =
            trim(body.substr(colon + 1, valueEnd - colon - 1));

        if (!key.empty())
        {
            values[key] = unquote(rawValue);
        }

        if (valueEnd >= body.size() ||
            body[valueEnd] == '}')
        {
            break;
        }

        position = valueEnd + 1;
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

    request.avoidRepeats =
        parseBool(values, "avoidRepeats", false);

    request.allowedRepeats =
        parseInt(values, "allowedRepeats", 0);

    request.repeatsWithinDays =
        parseInt(values, "repeatsWithinDays", 0);

    request.compareTitle =
        parseBool(values, "compareTitle", false);

    request.compareSubtitle =
        parseBool(values, "compareSubtitle", false);

    request.compareSummary =
        parseBool(values, "compareSummary", false);

    request.compareCategories =
        parseBool(values, "compareCategories", false);

    request.compareTime =
        parseBool(values, "compareTime", false);

    request.useSeriesRecording =
        parseBool(values, "useSeriesRecording", false);

    request.keepRecordings =
        parseInt(values, "keepRecordings", 0);

    request.deleteMode =
        parseInt(values, "deleteMode", 0);

    request.searchTimerAction =
        parseInt(values, "searchTimerAction", 0);

    request.blacklistMode =
        parseInt(values, "blacklistMode", 0);

    request.blacklistIds =
        getValue(values, "blacklistIds");

    request.matchMode =
        parseInt(values, "matchMode", 0);

    request.matchCase =
        parseBool(values, "matchCase", false);

    request.matchTolerance =
        parseInt(values, "matchTolerance", 0);

    request.summaryMatch =
        parseInt(values, "summaryMatch", 0);

    request.useExtendedEpgInfo =
        parseBool(values, "useExtendedEpgInfo", false);

    request.extendedEpgInfo =
        getValue(values, "extendedEpgInfo");

    request.ignoreMissingEpgCategories =
        parseBool(values, "ignoreMissingEpgCategories", false);

    request.contentDescriptors =
        getValue(values, "contentDescriptors");

    request.useInFavorites =
        parseBool(values, "useInFavorites", false);

    request.activeFrom =
        getValue(values, "activeFrom");

    request.activeUntil =
        getValue(values, "activeUntil");

    request.pauseOnRecordings =
        parseBool(values, "pauseOnRecordings", false);

    request.switchMinutesBefore =
        parseInt(values, "switchMinutesBefore", 0);

    request.unmuteSoundOnSwitch =
        parseBool(values, "unmuteSoundOnSwitch", false);

    request.deleteRecordingsAfterDays =
        parseInt(values, "deleteRecordingsAfterDays", 0);

    request.deleteAfterCountRecordings =
        parseInt(values, "deleteAfterCountRecordings", 0);

    request.deleteAfterDaysOfFirstRecording =
        parseInt(values, "deleteAfterDaysOfFirstRecording", 0);

    return request;
}

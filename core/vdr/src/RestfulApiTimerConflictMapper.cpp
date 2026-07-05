#include "RestfulApiTimerConflictMapper.h"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string trim(const std::string& value)
{
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin])))
    {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }

    return value.substr(begin, end - begin);
}

std::vector<std::string> split(const std::string& value, char separator)
{
    std::vector<std::string> parts;
    std::string current;

    for (char ch : value)
    {
        if (ch == separator)
        {
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }

    parts.push_back(current);
    return parts;
}

long long parseLongLong(const std::string& value)
{
    try
    {
        return std::stoll(trim(value));
    }
    catch (...)
    {
        return 0;
    }
}

int parseInt(const std::string& value)
{
    try
    {
        return std::stoi(trim(value));
    }
    catch (...)
    {
        return 0;
    }
}

int parseIntegerField(const std::string& json, const std::string& fieldName)
{
    const std::string quotedField = "\"" + fieldName + "\"";
    const std::size_t fieldPosition = json.find(quotedField);

    if (fieldPosition == std::string::npos)
    {
        return 0;
    }

    const std::size_t colonPosition =
        json.find(':', fieldPosition + quotedField.size());

    if (colonPosition == std::string::npos)
    {
        return 0;
    }

    std::size_t valueStart = colonPosition + 1;

    while (valueStart < json.size() &&
           std::isspace(static_cast<unsigned char>(json[valueStart])))
    {
        ++valueStart;
    }

    bool negative = false;
    if (valueStart < json.size() && json[valueStart] == '-')
    {
        negative = true;
        ++valueStart;
    }

    std::size_t valueEnd = valueStart;

    while (valueEnd < json.size() &&
           std::isdigit(static_cast<unsigned char>(json[valueEnd])))
    {
        ++valueEnd;
    }

    if (valueEnd == valueStart)
    {
        return 0;
    }

    const int number =
        parseInt(json.substr(valueStart, valueEnd - valueStart));

    return negative ? -number : number;
}

bool parseBoolField(const std::string& json, const std::string& fieldName)
{
    const std::string quotedField = "\"" + fieldName + "\"";
    const std::size_t fieldPosition = json.find(quotedField);

    if (fieldPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t colonPosition =
        json.find(':', fieldPosition + quotedField.size());

    if (colonPosition == std::string::npos)
    {
        return false;
    }

    std::size_t valueStart = colonPosition + 1;

    while (valueStart < json.size() &&
           std::isspace(static_cast<unsigned char>(json[valueStart])))
    {
        ++valueStart;
    }

    return json.compare(valueStart, 4, "true") == 0;
}

std::vector<std::string> parseStringArrayField(
    const std::string& json,
    const std::string& fieldName)
{
    std::vector<std::string> values;

    const std::string quotedField = "\"" + fieldName + "\"";
    const std::size_t fieldPosition = json.find(quotedField);

    if (fieldPosition == std::string::npos)
    {
        return values;
    }

    const std::size_t colonPosition =
        json.find(':', fieldPosition + quotedField.size());

    if (colonPosition == std::string::npos)
    {
        return values;
    }

    std::size_t position =
        json.find('[', colonPosition + 1);

    if (position == std::string::npos)
    {
        return values;
    }

    ++position;

    while (position < json.size())
    {
        while (position < json.size() &&
               std::isspace(static_cast<unsigned char>(json[position])))
        {
            ++position;
        }

        if (position >= json.size() || json[position] == ']')
        {
            break;
        }

        if (json[position] != '"')
        {
            ++position;
            continue;
        }

        ++position;
        std::string value;

        while (position < json.size())
        {
            const char ch = json[position];

            if (ch == '\\' && position + 1 < json.size())
            {
                value.push_back(json[position + 1]);
                position += 2;
                continue;
            }

            if (ch == '"')
            {
                ++position;
                break;
            }

            value.push_back(ch);
            ++position;
        }

        values.push_back(value);
    }

    return values;
}

std::vector<int> parseTimerIndexList(const std::string& value)
{
    std::vector<int> indices;

    for (const std::string& part : split(value, '#'))
    {
        const int index = parseInt(part);
        if (index > 0)
        {
            indices.push_back(index);
        }
    }

    return indices;
}

VdrTimerConflictEntry parseConflictEntry(const std::string& value)
{
    VdrTimerConflictEntry entry;

    const std::vector<std::string> parts =
        split(value, '|');

    if (!parts.empty())
    {
        entry.timerIndex = parseInt(parts[0]);
    }

    if (parts.size() > 1)
    {
        entry.percentage = parseInt(parts[1]);
    }

    if (parts.size() > 2)
    {
        entry.concurrentTimerIndices =
            parseTimerIndexList(parts[2]);
    }

    if (parts.size() > 3)
    {
        entry.remoteServer = parts[3];
    }

    return entry;
}

VdrTimerConflict parseConflictLine(const std::string& raw)
{
    VdrTimerConflict conflict;
    conflict.raw = raw;

    const std::vector<std::string> parts =
        split(raw, ':');

    if (parts.empty())
    {
        return conflict;
    }

    conflict.conflictTime = parseLongLong(parts[0]);

    for (std::size_t index = 1; index < parts.size(); ++index)
    {
        const std::string part = trim(parts[index]);
        if (!part.empty())
        {
            conflict.entries.push_back(
                parseConflictEntry(part));
        }
    }

    return conflict;
}

} // namespace

VdrTimerConflictReport RestfulApiTimerConflictMapper::parseReport(
    const std::string& json,
    int statusCode)
{
    VdrTimerConflictReport report;
    report.source = "restfulapi-epgsearch";

    if (statusCode != 200)
    {
        report.available = false;
        report.error = "HTTP " + std::to_string(statusCode);
        return report;
    }

    report.available = true;
    report.checkAdvised =
        parseBoolField(json, "check_advised");

    for (const std::string& rawConflict :
         parseStringArrayField(json, "conflicts"))
    {
        report.conflicts.push_back(
            parseConflictLine(rawConflict));
    }

    report.count =
        parseIntegerField(json, "count");

    report.total =
        parseIntegerField(json, "total");

    if (report.count == 0 && !report.conflicts.empty())
    {
        report.count =
            static_cast<int>(report.conflicts.size());
    }

    if (report.total == 0 && !report.conflicts.empty())
    {
        report.total =
            static_cast<int>(report.conflicts.size());
    }

    return report;
}

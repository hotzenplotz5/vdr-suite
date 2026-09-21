#include "SearchTimerWorkflowValidationRequestParser.h"

#include <map>
#include <string>

namespace
{

std::string unquote(
    const std::string& value)
{
    if (value.size() >= 2 &&
        value.front() == '"' &&
        value.back() == '"')
    {
        return value.substr(1, value.size() - 2);
    }

    return value;
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

SearchTimerWorkflowExecutionMode parseExecutionMode(
    const std::map<std::string, std::string>& values)
{
    std::string mode =
        getValue(values, "executionMode");

    if (mode.empty())
    {
        mode =
            getValue(values, "mode");
    }

    if (mode == "dryRun" ||
        mode == "dry-run" ||
        mode == "dryrun")
    {
        return SearchTimerWorkflowExecutionMode::DryRun;
    }

    if (mode == "execute" ||
        mode == "real")
    {
        return SearchTimerWorkflowExecutionMode::Execute;
    }

    return SearchTimerWorkflowExecutionMode::Prepare;
}

} // namespace

SearchTimerWorkflowRequest SearchTimerWorkflowValidationRequestParser::parse(
    const std::string& body) const
{
    const std::map<std::string, std::string> values =
        parseFlatObject(body);

    std::string operation =
        getValue(values, "operation");

    if (operation.empty())
    {
        operation =
            getValue(values, "action");
    }

    const std::string backendId =
        getValue(values, "backendId");

    const std::string backendNativeId =
        getValue(values, "backendNativeId");

    const std::string name =
        getValue(values, "name");

    const std::string query =
        getValue(values, "query");

    const bool active =
        parseBool(values, "active", true);

    const bool compareTitle =
        parseBool(values, "compareTitle", false);

    const bool compareSubtitle =
        parseBool(values, "compareSubtitle", false);

    const bool compareSummary =
        parseBool(values, "compareSummary", false);

    const bool compareCategories =
        parseBool(values, "compareCategories", false);

    const SearchTimerWorkflowExecutionMode executionMode =
        parseExecutionMode(values);

    if (operation == "list")
    {
        return SearchTimerWorkflowRequest::list(backendId)
            .withExecutionMode(executionMode);
    }

    if (operation == "preview")
    {
        return SearchTimerWorkflowRequest::preview(
            backendId,
            name,
            query)
            .withExecutionMode(executionMode);
    }

    if (operation == "create")
    {
        return SearchTimerWorkflowRequest::create(
            backendId,
            name,
            query,
            active)
            .withCompareFields(
                compareTitle,
                compareSubtitle,
                compareSummary,
                compareCategories)
            .withExecutionMode(executionMode);
    }

    if (operation == "readback")
    {
        return SearchTimerWorkflowRequest::readback(
            backendId,
            backendNativeId)
            .withExecutionMode(executionMode);
    }

    if (operation == "update")
    {
        return SearchTimerWorkflowRequest::update(
            backendId,
            backendNativeId,
            name,
            query,
            active)
            .withCompareFields(
                compareTitle,
                compareSubtitle,
                compareSummary,
                compareCategories)
            .withExecutionMode(executionMode);
    }

    if (operation == "delete" ||
        operation == "remove")
    {
        return SearchTimerWorkflowRequest::remove(
            backendId,
            backendNativeId)
            .withExecutionMode(executionMode);
    }

    return SearchTimerWorkflowRequest()
        .withExecutionMode(executionMode);
}

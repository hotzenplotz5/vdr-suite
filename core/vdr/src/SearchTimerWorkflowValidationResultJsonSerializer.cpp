#include "SearchTimerWorkflowValidationResultJsonSerializer.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{

void appendQuoted(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';

    for (const char character : value)
    {
        switch (character)
        {
        case '"':
            json << "\\\"";
            break;
        case '\\':
            json << "\\\\";
            break;
        case '\n':
            json << "\\n";
            break;
        case '\r':
            json << "\\r";
            break;
        case '\t':
            json << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(character) < 0x20)
            {
                json << ' ';
            }
            else
            {
                json << character;
            }
            break;
        }
    }

    json << '"';
}

void appendStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << "[";

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        appendQuoted(json, values.at(index));
    }

    json << "]";
}

const char* boolText(
    bool value)
{
    return value ? "true" : "false";
}

const char* operationText(
    SearchTimerWorkflowOperation operation)
{
    switch (operation)
    {
    case SearchTimerWorkflowOperation::List:
        return "list";
    case SearchTimerWorkflowOperation::Preview:
        return "preview";
    case SearchTimerWorkflowOperation::Create:
        return "create";
    case SearchTimerWorkflowOperation::Readback:
        return "readback";
    case SearchTimerWorkflowOperation::Update:
        return "update";
    case SearchTimerWorkflowOperation::Delete:
        return "delete";
    case SearchTimerWorkflowOperation::Unknown:
        return "unknown";
    }

    return "unknown";
}

}

std::string SearchTimerWorkflowValidationResultJsonSerializer::serialize(
    const SearchTimerWorkflowValidationResult& result) const
{
    std::ostringstream json;

    json << "{";
    json << "\"valid\":" << boolText(result.valid);
    json << ",\"operation\":";
    appendQuoted(json, operationText(result.operation));
    json << ",\"readOnly\":" << boolText(result.readOnly);
    json << ",\"writeOperation\":" << boolText(result.writeOperation);
    json << ",\"wantsReadbackAfterWrite\":"
         << boolText(result.wantsReadbackAfterWrite);
    json << ",\"backendId\":";
    appendQuoted(json, result.backendId);
    json << ",\"backendNativeId\":";
    appendQuoted(json, result.backendNativeId);
    json << ",\"warnings\":";
    appendStringArray(json, result.warnings);
    json << ",\"errors\":";
    appendStringArray(json, result.errors);
    json << "}";

    return json.str();
}

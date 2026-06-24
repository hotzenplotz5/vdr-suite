#include "SearchTimerWorkflowExecutionResultJsonSerializer.h"

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
    json << '[';

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        appendQuoted(json, values[index]);
    }

    json << ']';
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

const char* executionModeText(
    SearchTimerWorkflowExecutionMode executionMode)
{
    switch (executionMode)
    {
    case SearchTimerWorkflowExecutionMode::DryRun:
        return "dryRun";
    case SearchTimerWorkflowExecutionMode::Prepare:
        return "prepare";
    case SearchTimerWorkflowExecutionMode::Execute:
        return "execute";
    }

    return "prepare";
}

const char* stepText(
    SearchTimerWorkflowExecutionStep step)
{
    switch (step)
    {
    case SearchTimerWorkflowExecutionStep::None:
        return "none";
    case SearchTimerWorkflowExecutionStep::List:
        return "list";
    case SearchTimerWorkflowExecutionStep::Preview:
        return "preview";
    case SearchTimerWorkflowExecutionStep::Create:
        return "create";
    case SearchTimerWorkflowExecutionStep::Readback:
        return "readback";
    case SearchTimerWorkflowExecutionStep::Update:
        return "update";
    case SearchTimerWorkflowExecutionStep::Delete:
        return "delete";
    }

    return "none";
}

} // namespace

std::string SearchTimerWorkflowExecutionResultJsonSerializer::serialize(
    const SearchTimerWorkflowExecutionResult& result) const
{
    std::ostringstream json;

    json << "{";
    json << "\"success\":" << boolText(result.success);
    json << ",\"executed\":" << boolText(result.executed);
    json << ",\"blocked\":" << boolText(result.blocked);
    json << ",\"dryRunOnly\":" << boolText(result.dryRunOnly);
    json << ",\"confirmationProvided\":"
         << boolText(result.confirmationProvided);
    json << ",\"requiresExplicitOperatorConfirmation\":"
         << boolText(result.requiresExplicitOperatorConfirmation);
    json << ",\"requiresBackendReadback\":"
         << boolText(result.requiresBackendReadback);
    json << ",\"commandRequestMapped\":"
         << boolText(result.commandRequestMapped);
    json << ",\"realExecutionEnabled\":"
         << boolText(result.realExecutionEnabled);
    json << ",\"realExecutionPolicyAllowed\":"
         << boolText(result.realExecutionPolicyAllowed);
    json << ",\"executorOptInProvided\":"
         << boolText(result.executorOptInProvided);
    json << ",\"dispatchStage\":";
    appendQuoted(json, result.dispatchStage);
    json << ",\"executionMode\":";
    appendQuoted(json, executionModeText(result.executionMode));
    json << ",\"operation\":";
    appendQuoted(json, operationText(result.operation));
    json << ",\"primaryStep\":";
    appendQuoted(json, stepText(result.primaryStep));
    json << ",\"followUpStep\":";
    appendQuoted(json, stepText(result.followUpStep));
    json << ",\"backendId\":";
    appendQuoted(json, result.backendId);
    json << ",\"backendNativeId\":";
    appendQuoted(json, result.backendNativeId);
    json << ",\"message\":";
    appendQuoted(json, result.message);
    json << ",\"warnings\":";
    appendStringArray(json, result.warnings);
    json << ",\"errors\":";
    appendStringArray(json, result.errors);
    json << "}";

    return json.str();
}

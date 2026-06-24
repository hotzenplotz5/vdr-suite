#include "SearchTimerWorkflowExecutionPlanJsonSerializer.h"

#include <sstream>
#include <string>

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

std::string SearchTimerWorkflowExecutionPlanJsonSerializer::serialize(
    const SearchTimerWorkflowExecutionPlan& plan) const
{
    std::ostringstream json;

    json << "{";
    json << "\"valid\":" << boolText(plan.valid());
    json << ",\"operation\":";
    appendQuoted(json, operationText(plan.operation()));
    json << ",\"primaryStep\":";
    appendQuoted(json, stepText(plan.primaryStep()));
    json << ",\"followUpStep\":";
    appendQuoted(json, stepText(plan.followUpStep()));
    json << ",\"hasExecutionWork\":" << boolText(plan.hasExecutionWork());
    json << ",\"hasFollowUpStep\":" << boolText(plan.hasFollowUpStep());
    json << ",\"readOnly\":" << boolText(plan.readOnly());
    json << ",\"writeOperation\":" << boolText(plan.writeOperation());
    json << ",\"requiresExplicitOperatorConfirmation\":"
         << boolText(plan.requiresExplicitOperatorConfirmation());
    json << ",\"requiresBackendReadback\":"
         << boolText(plan.requiresBackendReadback());
    json << ",\"backendId\":";
    appendQuoted(json, plan.backendId());
    json << ",\"backendNativeId\":";
    appendQuoted(json, plan.backendNativeId());
    json << ",\"name\":";
    appendQuoted(json, plan.name());
    json << ",\"query\":";
    appendQuoted(json, plan.query());
    json << ",\"active\":" << boolText(plan.active());
    json << "}";

    return json.str();
}

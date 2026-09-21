#include "SearchTimerWorkflowProductionExecutorHardeningPlanJsonSerializer.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{

const char* boolText(
    bool value)
{
    return value ? "true" : "false";
}

void appendQuoted(
    std::ostringstream& json,
    const std::string& value)
{
    json << "\"";

    for (char ch : value)
    {
        if (ch == '\\')
        {
            json << "\\\\";
        }
        else if (ch == '"')
        {
            json << "\\\"";
        }
        else
        {
            json << ch;
        }
    }

    json << "\"";
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

        appendQuoted(json, values[index]);
    }

    json << "]";
}

void appendRequirement(
    std::ostringstream& json,
    const SearchTimerWorkflowProductionExecutorHardeningRequirement& item)
{
    json << "{";
    json << "\"id\":";
    appendQuoted(json, item.id);
    json << ",\"title\":";
    appendQuoted(json, item.title);
    json << ",\"mandatory\":"
         << boolText(item.mandatory);
    json << ",\"satisfied\":"
         << boolText(item.satisfied);
    json << ",\"status\":";
    appendQuoted(json, item.status);
    json << ",\"detail\":";
    appendQuoted(json, item.detail);
    json << "}";
}

} // namespace

std::string
SearchTimerWorkflowProductionExecutorHardeningPlanJsonSerializer::serialize(
    const SearchTimerWorkflowProductionExecutorHardeningPlanResult& result) const
{
    std::ostringstream json;

    json << "{";
    json << "\"readyForProductionExecution\":"
         << boolText(result.readyForProductionExecution);
    json << ",\"requirements\":[";

    for (std::size_t index = 0; index < result.requirements.size(); ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        appendRequirement(
            json,
            result.requirements[index]);
    }

    json << "],\"blockers\":";
    appendStringArray(json, result.blockers);
    json << "}";

    return json.str();
}

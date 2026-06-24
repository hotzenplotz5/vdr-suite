#include "SearchTimerWorkflowRealExecutionReadinessReviewJsonSerializer.h"

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

} // namespace

std::string
SearchTimerWorkflowRealExecutionReadinessReviewJsonSerializer::serialize(
    const SearchTimerWorkflowRealExecutionReadinessReviewResult& result) const
{
    std::ostringstream json;

    json << "{";
    json << "\"readyForRealBackendExecution\":"
         << boolText(result.readyForRealBackendExecution);
    json << ",\"planExecutable\":"
         << boolText(result.planExecutable);
    json << ",\"writeOperation\":"
         << boolText(result.writeOperation);
    json << ",\"executeModeRequested\":"
         << boolText(result.executeModeRequested);
    json << ",\"explicitOperatorConfirmationProvided\":"
         << boolText(result.explicitOperatorConfirmationProvided);
    json << ",\"executorOptInProvided\":"
         << boolText(result.executorOptInProvided);
    json << ",\"executorInjected\":"
         << boolText(result.executorInjected);
    json << ",\"controlledTestInvocationOnly\":"
         << boolText(result.controlledTestInvocationOnly);
    json << ",\"productionRealExecutionEnabled\":"
         << boolText(result.productionRealExecutionEnabled);
    json << ",\"backendWriteAllowlistConfigured\":"
         << boolText(result.backendWriteAllowlistConfigured);
    json << ",\"backendWriteAllowed\":"
         << boolText(result.backendWriteAllowed);
    json << ",\"backendWritePermissionConfigured\":"
         << boolText(result.backendWritePermissionConfigured);
    json << ",\"backendWritePermitted\":"
         << boolText(result.backendWritePermitted);
    json << ",\"productionRealExecutionPolicyAvailable\":"
         << boolText(result.productionRealExecutionPolicyAvailable);
    json << ",\"readinessStage\":";
    appendQuoted(json, result.readinessStage);
    json << ",\"message\":";
    appendQuoted(json, result.message);
    json << ",\"satisfiedConditions\":";
    appendStringArray(json, result.satisfiedConditions);
    json << ",\"blockers\":";
    appendStringArray(json, result.blockers);
    json << "}";

    return json.str();
}

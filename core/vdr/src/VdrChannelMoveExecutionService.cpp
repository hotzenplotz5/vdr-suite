
#include "VdrChannelMoveExecutionService.h"

#include <sstream>

namespace
{
std::string buildCommand(
    int sourceNumber,
    int targetNumber)
{
    std::ostringstream stream;
    stream << "MOVC " << sourceNumber << " " << targetNumber;
    return stream.str();
}

VdrChannelMoveResult failedResult(
    const VdrChannelMoveRequest& request,
    const std::string& message,
    const std::vector<std::string>& errors)
{
    VdrChannelMoveResult result;
    result.success = false;
    result.dryRun = request.dryRun;
    result.backendId = request.backendId.empty() ? "default" : request.backendId;
    result.sourceNumber = request.sourceNumber;
    result.targetNumber = request.targetNumber;
    result.command = buildCommand(
        request.sourceNumber,
        request.targetNumber);
    result.message = message;
    result.errors = errors;
    return result;
}
}

VdrChannelMoveResult VdrChannelMoveExecutionService::execute(
    const VdrChannelMoveRequest& request,
    const VdrChannelMoveExecutorAdapterRegistry& registry,
    const BackendAccessDecision& accessDecision) const
{
    if (!accessDecision.allowed)
    {
        return failedResult(
            request,
            accessDecision.reason,
            accessDecision.errors);
    }

    if (request.sourceNumber <= 0)
    {
        return failedResult(
            request,
            "source channel number must be greater than zero",
            {"source channel number must be greater than zero"});
    }

    if (request.targetNumber <= 0)
    {
        return failedResult(
            request,
            "target channel number must be greater than zero",
            {"target channel number must be greater than zero"});
    }

    if (request.sourceNumber == request.targetNumber)
    {
        return failedResult(
            request,
            "source and target channel number are identical",
            {"source and target channel number are identical"});
    }

    if (request.dryRun)
    {
        VdrChannelMoveResult result;
        result.success = true;
        result.dryRun = true;
        result.backendId = request.backendId.empty() ? "default" : request.backendId;
        result.sourceNumber = request.sourceNumber;
        result.targetNumber = request.targetNumber;
        result.command = buildCommand(
            request.sourceNumber,
            request.targetNumber);
        result.message = "dry-run: channel move command validated";
        return result;
    }

    const VdrChannelMoveExecutorAdapterLookupResult resolvedAdapter =
        registry.findAdapter(
            request.backendId.empty() ? "default" : request.backendId);

    if (!resolvedAdapter.found || !resolvedAdapter.adapter)
    {
        return failedResult(
            request,
            resolvedAdapter.message,
            {resolvedAdapter.message});
    }

    return resolvedAdapter.adapter->executor().moveChannel(request);
}

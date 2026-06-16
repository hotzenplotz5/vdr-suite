#include "RecordingActionValidationService.h"


namespace
{
std::string capabilityForAction(
    RecordingActionType type)
{
    switch (type)
    {
        case RecordingActionType::Move:
            return "recordings.action.move";
        case RecordingActionType::Rename:
            return "recordings.action.rename";
        case RecordingActionType::Delete:
            return "recordings.action.delete";
        case RecordingActionType::MetadataRefresh:
            return "recordings.action.metadata.refresh";
        default:
            return "";
    }
}

std::string permissionForAction(
    RecordingActionType type)
{
    switch (type)
    {
        case RecordingActionType::Move:
            return "recordings.action.move";
        case RecordingActionType::Rename:
            return "recordings.action.rename";
        case RecordingActionType::Delete:
            return "recordings.action.delete";
        case RecordingActionType::MetadataRefresh:
            return "recordings.action.metadata.refresh";
        default:
            return "";
    }
}

bool hasParameter(
    const RecordingActionRequest& request,
    const std::string& key)
{
    const auto iterator =
        request.parameters.find(key);

    return iterator != request.parameters.end() &&
           !iterator->second.empty();
}
}

RecordingActionValidationResult RecordingActionValidationService::validate(
    const RecordingActionRequest& request) const
{
    RecordingActionValidationResult result;

    result.dryRun = request.dryRun;
    result.wouldCreateJob = !request.dryRun;
    result.backendId = request.backendId;
    result.recordingId = request.recordingId;

    if (request.backendId.empty())
    {
        result.errors.push_back("backendId is required");
    }

    if (request.recordingId.empty())
    {
        result.errors.push_back("recordingId is required");
    }

    if (request.type == RecordingActionType::Unknown)
    {
        result.errors.push_back("recording action type is required");
    }

    const std::string capability =
        capabilityForAction(request.type);

    if (!capability.empty())
    {
        result.requiredCapabilities.push_back(capability);
    }

    const std::string permission =
        permissionForAction(request.type);

    if (!permission.empty())
    {
        result.requiredPermissions.push_back(permission);
    }

    if (request.type == RecordingActionType::Move &&
        !hasParameter(request, "targetPath"))
    {
        result.errors.push_back("targetPath is required for move");
    }

    if (request.type == RecordingActionType::Rename &&
        !hasParameter(request, "newName"))
    {
        result.errors.push_back("newName is required for rename");
    }

    if (request.dryRun)
    {
        result.warnings.push_back("dry-run only");
    }

    result.valid =
        result.errors.empty();

    return result;
}

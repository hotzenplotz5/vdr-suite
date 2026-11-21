#pragma once

#include "RecordingAction.h"

#include <algorithm>
#include <string>
#include <vector>

struct RecordingActionPermissionSet
{
    std::vector<std::string> permissions;

    bool contains(
        const std::string& permission) const
    {
        return std::find(
                   permissions.begin(),
                   permissions.end(),
                   permission) != permissions.end();
    }

    void add(
        const std::string& permission)
    {
        if (!contains(permission))
        {
            permissions.push_back(permission);
        }
    }
};

struct RecordingActionPermissionCheckResult
{
    bool allowed = false;
    std::string requiredPermission;
    std::vector<std::string> missingPermissions;
};

class RecordingActionPermissionContract
{
public:
    std::string requiredPermission(
        RecordingActionType action) const
    {
        switch (action)
        {
        case RecordingActionType::Check:
            return "recording.permission.check";
        case RecordingActionType::Repair:
            return "recording.permission.repair";
        case RecordingActionType::Shrink:
            return "recording.permission.shrink";
        case RecordingActionType::Cut:
            return "recording.permission.cut";
        case RecordingActionType::Pes2Ts:
            return "recording.permission.pes2ts";
        case RecordingActionType::Rename:
            return "recording.permission.rename";
        case RecordingActionType::Move:
            return "recording.permission.move";
        case RecordingActionType::Delete:
            return "recording.permission.delete";
        case RecordingActionType::MetadataRefresh:
            return "recording.permission.metadata-refresh";
        case RecordingActionType::Unknown:
            return "";
        }

        return "";
    }

    RecordingActionPermissionCheckResult check(
        RecordingActionType action,
        const RecordingActionPermissionSet& permissionSet) const
    {
        RecordingActionPermissionCheckResult result;
        result.requiredPermission = requiredPermission(action);

        if (result.requiredPermission.empty())
        {
            result.allowed = false;
            result.missingPermissions.push_back(
                "recording.permission.unknown");
            return result;
        }

        result.allowed =
            permissionSet.contains(result.requiredPermission);

        if (!result.allowed)
        {
            result.missingPermissions.push_back(
                result.requiredPermission);
        }

        return result;
    }

    RecordingActionPermissionSet readOnlyPermissions() const
    {
        RecordingActionPermissionSet permissionSet;
        permissionSet.add("recording.permission.view");
        return permissionSet;
    }

    RecordingActionPermissionSet restfulApiMutationPermissions() const
    {
        RecordingActionPermissionSet permissionSet;
        permissionSet.add("recording.permission.view");
        permissionSet.add("recording.permission.move");
        permissionSet.add("recording.permission.rename");
        permissionSet.add("recording.permission.delete");
        return permissionSet;
    }

    RecordingActionPermissionSet liveReferencePermissions() const
    {
        RecordingActionPermissionSet permissionSet;
        permissionSet.add("recording.permission.view");
        permissionSet.add("recording.permission.cut");
        permissionSet.add("recording.permission.rename");
        permissionSet.add("recording.permission.move");
        permissionSet.add("recording.permission.delete");
        permissionSet.add("recording.permission.metadata-refresh");
        return permissionSet;
    }
};

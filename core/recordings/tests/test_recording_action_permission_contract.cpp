#include "RecordingActionPermissionContract.h"

#include <cassert>
#include <string>

int main()
{
    RecordingActionPermissionContract contract;

    assert(contract.requiredPermission(RecordingActionType::Move) ==
           "recording.permission.move");
    assert(contract.requiredPermission(RecordingActionType::Rename) ==
           "recording.permission.rename");
    assert(contract.requiredPermission(RecordingActionType::Delete) ==
           "recording.permission.delete");
    assert(contract.requiredPermission(RecordingActionType::Cut) ==
           "recording.permission.cut");
    assert(contract.requiredPermission(RecordingActionType::MetadataRefresh) ==
           "recording.permission.metadata-refresh");
    assert(contract.requiredPermission(RecordingActionType::Unknown).empty());

    RecordingActionPermissionSet permissions;
    permissions.add("recording.permission.move");
    permissions.add("recording.permission.move");
    permissions.add("recording.permission.delete");

    assert(permissions.permissions.size() == 2);
    assert(permissions.contains("recording.permission.move"));
    assert(permissions.contains("recording.permission.delete"));
    assert(!permissions.contains("recording.permission.rename"));

    {
        const RecordingActionPermissionCheckResult result =
            contract.check(RecordingActionType::Move, permissions);

        assert(result.allowed);
        assert(result.requiredPermission == "recording.permission.move");
        assert(result.missingPermissions.empty());
    }

    {
        const RecordingActionPermissionCheckResult result =
            contract.check(RecordingActionType::Rename, permissions);

        assert(!result.allowed);
        assert(result.requiredPermission == "recording.permission.rename");
        assert(result.missingPermissions.size() == 1);
        assert(result.missingPermissions.at(0) ==
               "recording.permission.rename");
    }

    {
        const RecordingActionPermissionCheckResult result =
            contract.check(RecordingActionType::Unknown, permissions);

        assert(!result.allowed);
        assert(result.requiredPermission.empty());
        assert(result.missingPermissions.size() == 1);
        assert(result.missingPermissions.at(0) ==
               "recording.permission.unknown");
    }

    {
        const RecordingActionPermissionSet readOnly =
            contract.readOnlyPermissions();

        assert(readOnly.contains("recording.permission.view"));
        assert(!readOnly.contains("recording.permission.move"));
        assert(!readOnly.contains("recording.permission.delete"));
    }

    {
        const RecordingActionPermissionSet restful =
            contract.restfulApiMutationPermissions();

        assert(restful.contains("recording.permission.view"));
        assert(restful.contains("recording.permission.move"));
        assert(restful.contains("recording.permission.rename"));
        assert(restful.contains("recording.permission.delete"));
        assert(!restful.contains("recording.permission.cut"));
    }

    {
        const RecordingActionPermissionSet live =
            contract.liveReferencePermissions();

        assert(live.contains("recording.permission.view"));
        assert(live.contains("recording.permission.cut"));
        assert(live.contains("recording.permission.rename"));
        assert(live.contains("recording.permission.move"));
        assert(live.contains("recording.permission.delete"));
        assert(live.contains("recording.permission.metadata-refresh"));
    }

    return 0;
}

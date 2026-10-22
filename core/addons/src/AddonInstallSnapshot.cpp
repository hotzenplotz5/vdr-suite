#include "AddonInstallSnapshot.h"
#include "AddonInstalledManifest.h"

#include <utility>

namespace vdrsuite::addons {

MediaToolsInstallSnapshot inspectMediaToolsInstallSnapshot(
    const std::string& root)
{
    const RootApprovalResult approval = readRootMediaToolsApproval(root);
    if (!approval.approved())
        return {};
    if (!installedMediaToolsManifestMatches(root, approval.version))
        return {InstallSnapshotReason::invalidMetadata,
                InstallOrigin::absent, {}, {}, {}};

    HandlerDescriptorResult binary =
        inspectPinnedMediaToolsHandler(root, approval.sha256);
    if (!binary.verified())
        return {InstallSnapshotReason::invalidHandler,
                InstallOrigin::absent, {}, {}, {}};

    // A root-owned approval may be revoked or replaced while the handler
    // bytes are being hashed. Reject any change before exposing the snapshot.
    const RootApprovalResult current = readRootMediaToolsApproval(root);
    if (!current.approved() || current.origin != approval.origin ||
        current.version != approval.version || current.sha256 != approval.sha256 ||
        !installedMediaToolsManifestMatches(root, current.version))
        return {InstallSnapshotReason::missingOrRevokedApproval,
                InstallOrigin::absent, {}, {}, {}};

    // The ONLY positive result holds the verified open descriptor itself.
    // Reopening a path later would invalidate this guarantee.
    return {InstallSnapshotReason::verifiedBytesOnly,
            approval.origin, approval.version, approval.sha256,
            std::move(binary.descriptor)};
}

} // namespace vdrsuite::addons

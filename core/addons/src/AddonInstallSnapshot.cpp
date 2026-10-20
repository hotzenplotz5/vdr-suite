#include "AddonInstallSnapshot.h"

#include <utility>

namespace vdrsuite::addons {

MediaToolsInstallSnapshot inspectMediaToolsInstallSnapshot(
    const std::string& root)
{
    const RootApprovalResult approval = readRootMediaToolsApproval(root);
    if (!approval.approved())
        return {};

    HandlerDescriptorResult binary =
        inspectPinnedMediaToolsHandler(root, approval.sha256);
    if (!binary.verified())
        return {InstallSnapshotReason::invalidHandler,
                InstallOrigin::absent, {}, {}, {}};

    // The ONLY positive result holds the verified open descriptor itself.
    // Reopening a path later would invalidate this guarantee.
    return {InstallSnapshotReason::verifiedBytesOnly,
            approval.origin, approval.version, approval.sha256,
            std::move(binary.descriptor)};
}

} // namespace vdrsuite::addons

#pragma once

#include "AddonPinnedHandler.h"
#include "AddonRootApproval.h"

namespace vdrsuite::addons {

// Source- or Debian-installed handler bytes plus a separate root-admin
// approval, captured as one move-only, non-executing snapshot. Neither
// activation, capabilities, backend rights nor Agent authority are conveyed.
enum class InstallSnapshotReason {
    verifiedBytesOnly, missingOrRevokedApproval, invalidMetadata, invalidHandler
};

struct MediaToolsInstallSnapshot {
    InstallSnapshotReason reason = InstallSnapshotReason::missingOrRevokedApproval;
    InstallOrigin origin = InstallOrigin::absent;
    std::string approvedVersion;
    std::string approvedSha256;
    PinnedAddonHandler pinnedBinary;
    bool bytesVerified() const {
        return reason == InstallSnapshotReason::verifiedBytesOnly &&
               bool(pinnedBinary);
    }
};

// The root/path come from the local Core/Agent configuration, never an HTTP
// payload. This does not validate backend generation, installed manifest,
// signed package provenance or effective execution authorization.
MediaToolsInstallSnapshot inspectMediaToolsInstallSnapshot(
    const std::string& root);

} // namespace vdrsuite::addons
